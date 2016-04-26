/*
 * Copyright 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * FirewallControllerTest.cpp - unit tests for FirewallController.cpp
 */

#include <string>
#include <vector>
#include <stdio.h>

#include <gtest/gtest.h>

#include "FirewallController.h"
#include "IptablesBaseTest.h"


class FirewallControllerTest : public IptablesBaseTest {
protected:
    FirewallControllerTest() {
        FirewallController::execIptables = fakeExecIptables;
        FirewallController::execIptablesSilently = fakeExecIptables;
        FirewallController::execIptablesRestore = fakeExecIptablesRestore;
    }
    FirewallController mFw;

    std::string makeUidRules(const char *a, bool b, const std::vector<int32_t>& c) {
        return mFw.makeUidRules(a, b, c);
    }

    int createChain(const char* a, const char*b , FirewallType c) {
        return mFw.createChain(a, b, c);
    }
};


TEST_F(FirewallControllerTest, TestCreateWhitelistChain) {
    ExpectedIptablesCommands expected = {
        { V4V6, "-t filter -D INPUT -j fw_whitelist" },
        { V4V6, "-t filter -F fw_whitelist" },
        { V4V6, "-t filter -X fw_whitelist" },
        { V4V6, "-t filter -N fw_whitelist" },
        { V4V6, "-A fw_whitelist -p tcp --tcp-flags RST RST -j RETURN" },
        { V6,   "-A fw_whitelist -p icmpv6 --icmpv6-type packet-too-big -j RETURN" },
        { V6,   "-A fw_whitelist -p icmpv6 --icmpv6-type router-solicitation -j RETURN" },
        { V6,   "-A fw_whitelist -p icmpv6 --icmpv6-type router-advertisement -j RETURN" },
        { V6,   "-A fw_whitelist -p icmpv6 --icmpv6-type neighbour-solicitation -j RETURN" },
        { V6,   "-A fw_whitelist -p icmpv6 --icmpv6-type neighbour-advertisement -j RETURN" },
        { V6,   "-A fw_whitelist -p icmpv6 --icmpv6-type redirect -j RETURN" },
        { V4V6, "-A fw_whitelist -m owner --uid-owner 0-9999 -j RETURN" },
        { V4V6, "-A fw_whitelist -j DROP" },
    };
    createChain("fw_whitelist", "INPUT", WHITELIST);
    expectIptablesCommands(expected);
}

TEST_F(FirewallControllerTest, TestCreateBlacklistChain) {
    ExpectedIptablesCommands expected = {
        { V4V6, "-t filter -D INPUT -j fw_blacklist" },
        { V4V6, "-t filter -F fw_blacklist" },
        { V4V6, "-t filter -X fw_blacklist" },
        { V4V6, "-t filter -N fw_blacklist" },
        { V4V6, "-A fw_blacklist -p tcp --tcp-flags RST RST -j RETURN" },
    };
    createChain("fw_blacklist", "INPUT", BLACKLIST);
    expectIptablesCommands(expected);
}

TEST_F(FirewallControllerTest, TestSetStandbyRule) {
    ExpectedIptablesCommands expected = {
        { V4V6, "-D fw_standby -m owner --uid-owner 12345 -j DROP" }
    };
    mFw.setUidRule(STANDBY, 12345, ALLOW);
    expectIptablesCommands(expected);

    expected = {
        { V4V6, "-A fw_standby -m owner --uid-owner 12345 -j DROP" }
    };
    mFw.setUidRule(STANDBY, 12345, DENY);
    expectIptablesCommands(expected);
}

TEST_F(FirewallControllerTest, TestSetDozeRule) {
    ExpectedIptablesCommands expected = {
        { V4V6, "-I fw_dozable -m owner --uid-owner 54321 -j RETURN" }
    };
    mFw.setUidRule(DOZABLE, 54321, ALLOW);
    expectIptablesCommands(expected);

    expected = {
        { V4V6, "-D fw_dozable -m owner --uid-owner 54321 -j RETURN" }
    };
    mFw.setUidRule(DOZABLE, 54321, DENY);
    expectIptablesCommands(expected);
}

TEST_F(FirewallControllerTest, TestReplaceWhitelistUidRule) {
    std::string expected =
            "*filter\n"
            ":FW_whitechain -\n"
            "-A FW_whitechain -m owner --uid-owner 0-9999 -j RETURN\n"
            "-A FW_whitechain -m owner --uid-owner 10023 -j RETURN\n"
            "-A FW_whitechain -m owner --uid-owner 10059 -j RETURN\n"
            "-A FW_whitechain -m owner --uid-owner 10124 -j RETURN\n"
            "-A FW_whitechain -m owner --uid-owner 10111 -j RETURN\n"
            "-A FW_whitechain -m owner --uid-owner 110122 -j RETURN\n"
            "-A FW_whitechain -m owner --uid-owner 210153 -j RETURN\n"
            "-A FW_whitechain -m owner --uid-owner 210024 -j RETURN\n"
            "-A FW_whitechain -j DROP\n"
            "COMMIT\n\x04";

    std::vector<int32_t> uids = { 10023, 10059, 10124, 10111, 110122, 210153, 210024 };
    EXPECT_EQ(expected, makeUidRules("FW_whitechain", true, uids));
}

TEST_F(FirewallControllerTest, TestReplaceBlacklistUidRule) {
    std::string expected =
            "*filter\n"
            ":FW_blackchain -\n"
            "-A FW_blackchain -m owner --uid-owner 10023 -j DROP\n"
            "-A FW_blackchain -m owner --uid-owner 10059 -j DROP\n"
            "-A FW_blackchain -m owner --uid-owner 10124 -j DROP\n"
            "-A FW_blackchain -j RETURN\n"
            "COMMIT\n\x04";

    std::vector<int32_t> uids = { 10023, 10059, 10124 };
    EXPECT_EQ(expected, makeUidRules("FW_blackchain", false, uids));
}
