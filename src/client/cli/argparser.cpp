/*
 * Copyright (C) 2017-2020 Canonical, Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <multipass/cli/alias_dict.h>
#include <multipass/cli/argparser.h>
#include <multipass/format.h>

#include <QFileInfo>
#include <QRegExp>

#include <algorithm>

/*
 * ArgParser - a wrapping of a QCommandLineParser where the concept of a "command"
 * is central. It chooses which Command is requested (if any) and allows that Command
 * to continue the parsing of the arguments.
 *
 * This class also manages both the global and Command-specific help text, by massaging
 * the natural output of QCommandLineParser::helpText.
 */

namespace mp = multipass;
namespace cmd = multipass::cmd;

namespace
{
auto max_command_string_length(const std::vector<cmd::Command::UPtr>& commands)
{
    auto ret = 0ul;

    if (!commands.empty())
    {
        auto string_len_compare = [](const cmd::Command::UPtr& a, const cmd::Command::UPtr& b) {
            return a->name().length() < b->name().length();
        };
        const auto& max_elem = *std::max_element(commands.begin(), commands.end(), string_len_compare);
        ret = max_elem->name().length();
    }

    return ret;
}

auto format_into_column(const std::string& name, std::string::size_type column_size)
{
    return fmt::format("  {:<{}}  ", name, column_size);
}

QString format_short_help_for(const std::vector<cmd::Command::UPtr>& commands)
{
    const auto column_size = max_command_string_length(commands);
    QString output;
    for (const auto& c : commands)
    {
        output += QString::fromStdString(format_into_column(c->name(), column_size));
        output += c->short_help() + "\n";
    }
    return output;
}

auto verbosity_level_in(const QStringList& arguments)
{
    for (const QString& arg : arguments)
    {
        if (arg == QStringLiteral("-v") || arg == QStringLiteral("--verbose"))
            return 1;
        if (arg == QStringLiteral("-vv"))
            return 2;
        if (arg == QStringLiteral("-vvv"))
            return 3;
        if (QRegExp{"-vvvv+"}.exactMatch(arg))
            return 4;
    }
    return 0;
}
} // namespace

mp::ArgParser::ArgParser(const QStringList& arguments, const std::vector<cmd::Command::UPtr>& commands,
                         std::ostream& cout, std::ostream& cerr)
    : arguments(arguments), commands(commands), chosen_command(nullptr), help_requested(false), cout(cout), cerr(cerr)
{
}

mp::ParseCode mp::ArgParser::parse()
{
    QCommandLineOption help_option = parser.addHelpOption();
    QCommandLineOption verbose_option({"v", "verbose"},
                                      "Increase logging verbosity. Repeat the 'v' in the short option for more detail. "
                                      "Maximum verbosity is obtained with 4 (or more) v's, i.e. -vvvv.");
    QCommandLineOption version_option({"V", "version"}, "Show version details");
    version_option.setFlags(QCommandLineOption::HiddenFromHelp);
    parser.addOption(verbose_option);
    parser.addOption(version_option);

    // Register "command" as the first positional argument, will need to be removed from all help text later
    parser.addPositionalArgument("command", "The command to execute", "<command>");

    // First pass parse - only interested in determining the requested command, help or verbosity.
    const bool parser_result = parser.parse(arguments);

    if (parser.isSet(verbose_option))
    {
        verbosity_level = verbosity_level_in(arguments);
    }

    help_requested = parser.isSet(help_option);

    if (parser.positionalArguments().isEmpty() && !parser.isSet(version_option))
    {
        if (!parser_result)
        {
            cerr << qUtf8Printable(parser.errorText()) << "\n\n";
        }
        cout << qUtf8Printable(generalHelpText());
        return (help_requested) ? ParseCode::HelpRequested : ParseCode::CommandFail;
    }

    const QString requested_command =
        parser.isSet(version_option) ? QStringLiteral("version") : parser.positionalArguments().first();

    chosen_command = findCommand(requested_command);

    if (chosen_command)
    {
        return ParseCode::Ok;
    }

    if (help_requested)
    {
        cout << qUtf8Printable(generalHelpText());
        return ParseCode::HelpRequested;
    }

    // The given argument is not a command name. Before failing, see if it is an alias.
    AliasDict aliases; // Load the alias definitions.

    execute_alias = aliases.get_alias(requested_command.toStdString());

    if (execute_alias)
    {
        chosen_command = findCommand("exec");

        return ParseCode::Ok;
    }

    // Fall through
    cout << "Error: Unknown command or alias '" << qUtf8Printable(requested_command) << "' (try \"multipass help\")\n";
    return ParseCode::CommandLineError;
}

// Parse the command line again, this time with a particular Command in mind
mp::ParseCode mp::ArgParser::commandParse(cmd::Command* command)
{
    const bool parsedOk = parser.parse(arguments);
    if (!parsedOk)
    {
        cerr << qUtf8Printable(parser.errorText()) << '\n';
        return ParseCode::CommandLineError;
    }

    if (help_requested)
    {
        cout << qUtf8Printable(helpText(command));
        return ParseCode::HelpRequested;
    }
    return ParseCode::Ok;
}

mp::ReturnCode mp::ArgParser::returnCodeFrom(ParseCode parse_code) const
{
    switch (parse_code)
    {
    case ParseCode::CommandFail:
        return ReturnCode::CommandFail;
    case ParseCode::CommandLineError:
        return ReturnCode::CommandLineError;
    default:
        return ReturnCode::Ok;
    }
}

// This forces help to be printed using "help <command>"
void mp::ArgParser::forceCommandHelp()
{
    parser.clearPositionalArguments();

    // Need to add this back so helpText() can add the correct command for usage
    parser.addPositionalArgument("command", "The command to execute", "<command>");
    help_requested = true;
}

void mp::ArgParser::forceGeneralHelp()
{
    cout << qUtf8Printable(generalHelpText());
}

// Prints generic help
QString mp::ArgParser::generalHelpText()
{
    const QLatin1Char nl('\n');
    // The help text generated by QCommandLineParser needs some minor editing:
    //  1. Change "Displays this help." to "Display this help".
    //  2. Remove path prepended to the multipass executable name.
    //  3. remove the useless "Arguments" section listing just "command" (by clearing positional elements)
    //  4. compensate by editing the "usage" line to add "command" option back
    //  5. append a list of commands and their help text.
    parser.clearPositionalArguments();

    QString text = parser.helpText();
    text = text.replace(QCommandLineParser::tr("Displays this help."), QCommandLineParser::tr("Display this help"));

    QString cmd = text.split(' ').at(1);
    text = text.replace(cmd, QFileInfo(cmd).baseName());

    const int first_line_break_pos = text.indexOf('\n');
    text.insert(first_line_break_pos, " <command>");

    text += nl + QCommandLineParser::tr("Available commands:") + nl;
    text += format_short_help_for(commands);

    return text;
}

// Print command-specific help
QString mp::ArgParser::helpText(cmd::Command* command)
{
    parser.setApplicationDescription(command->description());
    QString text = parser.helpText();

    const QLatin1Char nl('\n');
    // The help text generated by QCommandLineParser needs some minor editing:
    //  1. Change "Displays this help." to "Display this help".
    //  2. Remove path prepended to the multipass executable name.
    //  3. replace <command> with the command, and swap with the [options] entry
    //  4. from the "Arguments" section, remove the "command" entry. Remove the entire
    //     section if it is empty afterwards.
    text = text.replace(QCommandLineParser::tr("Displays this help."), QCommandLineParser::tr("Display this help"));

    QString cmd = text.split(' ').at(1);
    text = text.replace(cmd, QFileInfo(cmd).baseName());

    text = text.replace(QCommandLineParser::tr("[options]") + " <command>",
                        QString::fromStdString(command->name()) + " " + QCommandLineParser::tr("[options]"));

    int start = text.indexOf(QRegExp("  command\\s*The command to execute"));
    int end = text.indexOf(nl, start);
    text = text.replace(start, end - start + 1, "");

    if (text.endsWith(QCommandLineParser::tr("Arguments:") + nl))
    {
        text = text.replace(nl + QCommandLineParser::tr("Arguments:") + nl, "");
    }

    return text;
}

/* Pass-through methods to the underlying QCommandLineParser */
void mp::ArgParser::setApplicationDescription(const QString& description)
{
    parser.setApplicationDescription(description);
}

bool mp::ArgParser::addOption(const QCommandLineOption& command_line_option)
{
    return parser.addOption(command_line_option);
}

bool mp::ArgParser::addOptions(const QList<QCommandLineOption>& options)
{
    return parser.addOptions(options);
}

void mp::ArgParser::addPositionalArgument(const QString& name, const QString& description, const QString& syntax)
{
    parser.addPositionalArgument(name, description, syntax);
}

cmd::Command* mp::ArgParser::chosenCommand() const
{
    return chosen_command;
}

cmd::Command* mp::ArgParser::findCommand(const QString& command) const
{
    for (const auto& c : commands)
    {
        for (const auto& alias : c->aliases())
        {
            if (command.toStdString() == alias)
            {
                return c.get();
            }
        }
    }

    return nullptr;
}

const std::vector<cmd::Command::UPtr>& mp::ArgParser::getCommands() const
{
    return commands;
}

bool mp::ArgParser::isSet(const QString& option) const
{
    return parser.isSet(option);
}

bool mp::ArgParser::isSet(const QCommandLineOption& option) const
{
    return parser.isSet(option);
}

QString mp::ArgParser::value(const QCommandLineOption& option) const
{
    return parser.value(option);
}

QString mp::ArgParser::value(const QString& option) const
{
    return parser.value(option);
}

QStringList mp::ArgParser::values(const QCommandLineOption& option) const
{
    return parser.values(option);
}

QStringList mp::ArgParser::positionalArguments() const
{
    // Remove the first "command" positional argument, if there, so calling Command sees just the
    // positional arguments it is interested in
    auto positionalArguments = parser.positionalArguments();
    if (positionalArguments.count() > 0)
    {
        positionalArguments.pop_front();
    }
    return positionalArguments;
}

QStringList mp::ArgParser::unknownOptionNames() const
{
    return parser.unknownOptionNames();
}

void mp::ArgParser::setVerbosityLevel(int verbosity)
{
    if (verbosity > 4 || verbosity < 0)
    {
        cerr << "Verbosity level is incorrect. Must be between 0 and 4.\n";
    }
    else if (verbosity_level != verbosity)
    {
        verbosity_level = verbosity;
    }
}

int mp::ArgParser::verbosityLevel() const
{
    return verbosity_level;
}

bool mp::ArgParser::containsArgument(const QString& argument) const
{
    return arguments.contains(argument);
}
