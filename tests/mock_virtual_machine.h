/*
 * Copyright (C) 2019-2021 Canonical, Ltd.
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

#ifndef MULTIPASS_MOCK_VIRTUAL_MACHINE_H
#define MULTIPASS_MOCK_VIRTUAL_MACHINE_H

#include <gmock/gmock.h>

#include "platform/backends/shared/base_virtual_machine.h"

namespace multipass
{
namespace test
{
// This mocks a virtual machine, but not the methods of the base virtual machine.
struct MockNonBaseVirtualMachine : public multipass::BaseVirtualMachine
{
    MockNonBaseVirtualMachine(const std::string vm_name) : BaseVirtualMachine{vm_name}
    {
    }

    MOCK_METHOD0(start, void());
    MOCK_METHOD0(stop, void());
    MOCK_METHOD0(shutdown, void());
    MOCK_METHOD0(suspend, void());
    MOCK_METHOD0(current_state, multipass::VirtualMachine::State());
    MOCK_METHOD0(ssh_port, int());
    MOCK_METHOD0(ssh_hostname, std::string());
    MOCK_METHOD1(ssh_hostname, std::string(std::chrono::milliseconds));
    MOCK_METHOD0(ssh_username, std::string());
    MOCK_METHOD0(management_ipv4, std::string());
    MOCK_METHOD0(ipv6, std::string());
    MOCK_METHOD0(ensure_vm_is_running, void());
    MOCK_METHOD1(wait_until_ssh_up, void(std::chrono::milliseconds));
    MOCK_METHOD0(update_state, void());
};

// This mocks a complete virtual machine, including the methods in the base virtual machine.
struct MockVirtualMachine : public MockNonBaseVirtualMachine
{
    MockVirtualMachine(const std::string vm_name) : MockNonBaseVirtualMachine{vm_name}
    {
        ON_CALL(*this, ssh_port()).WillByDefault(Return(42));
        ON_CALL(*this, ssh_hostname()).WillByDefault(Return("localhost"));
        ON_CALL(*this, ssh_hostname(_)).WillByDefault(Return("localhost"));
        ON_CALL(*this, ssh_username()).WillByDefault(Return("ubuntu"));
        ON_CALL(*this, management_ipv4()).WillByDefault(Return("0.0.0.0"));
        ON_CALL(*this, ipv6()).WillByDefault(Return("::/0"));
    }

    MOCK_METHOD1(get_all_ipv4, std::vector<std::string>(const SSHKeyProvider&));
};
} // namespace test
} // namespace multipass
#endif // MULTIPASS_MOCK_VIRTUAL_MACHINE_H
