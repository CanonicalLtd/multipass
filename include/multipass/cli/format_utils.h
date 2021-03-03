/*
 * Copyright (C) 2018-2021 Canonical, Ltd.
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

#ifndef MULTIPASS_FORMAT_UTILS_H
#define MULTIPASS_FORMAT_UTILS_H

#include <multipass/constants.h>
#include <multipass/rpc/multipass.grpc.pb.h>
#include <multipass/settings.h>

#include <fmt/format.h>

#include <algorithm>
#include <string>

namespace multipass
{
class Formatter;

namespace format
{
std::string status_string_for(const InstanceStatus& status);
std::string image_string_for(const multipass::FindReply_AliasInfo& alias);
Formatter* formatter_for(const std::string& format);

template <typename Instances>
Instances sorted(const Instances& instances);

void filter_aliases(google::protobuf::RepeatedPtrField<multipass::FindReply_AliasInfo>& aliases);

// Computes the column width needed to display all the elements of a range [begin, end). get_width is a function
// which takes as input the element in the range and returns its width in columns.
static constexpr auto column_width = [](const auto begin, const auto end, const auto get_width, int minimum_width = 0) {
    auto max_width =
        std::max_element(begin, end, [&get_width](auto& lhs, auto& rhs) { return get_width(lhs) < get_width(rhs); });
    return std::max(get_width(*max_width) + 2, minimum_width);
};
} // namespace format
}

template <typename Instances>
Instances multipass::format::sorted(const Instances& instances)
{
    if (instances.empty())
        return instances;

    auto ret = instances;
    const auto petenv_name = MP_SETTINGS.get(petenv_key).toStdString();
    std::sort(std::begin(ret), std::end(ret), [&petenv_name](const auto& a, const auto& b) {
        if (a.name() == petenv_name)
            return true;
        else if (b.name() == petenv_name)
            return false;
        else
            return a.name() < b.name();
    });

    return ret;
}

namespace fmt
{
template <>
struct formatter<multipass::FindReply_AliasInfo>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const multipass::FindReply_AliasInfo& a, FormatContext& ctx)
    {
        return format_to(ctx.out(), "{}", a.alias());
    }
};
} // namespace fmt

#endif // MULTIPASS_FORMAT_UTILS_H
