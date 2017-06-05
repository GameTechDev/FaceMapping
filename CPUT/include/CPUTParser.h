/////////////////////////////////////////////////////////////////////////////////////////////
// Copyright 2017 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or imlied.
// See the License for the specific language governing permissions and
// limitations under the License.
/////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CPUTPARSER_H
#define CPUTPARSER_H

#include <map>
#include <string>

class CommandParser {
public:
    ~CommandParser();
    void ParseConfigurationOptions(std::string arguments, std::string delimiter = "-");
    void ParseConfigurationOptions(int argc, char **argv, std::string delimiter = "-");
    void CleanConfigurationOptions(void);

    void AddParameter(std::string paramName, std::string paramValue);
    bool GetParameter(std::string arg);
    bool GetParameter(std::string arg, double *pOut);
    bool GetParameter(std::string arg, int *pOut);
    bool GetParameter(std::string arg, unsigned int *pOut);
    bool GetParameter(std::string arg, std::string *pOut);
    bool GetParameter(std::string arg, char *pOut);

private:
    std::map<std::string, std::string> m_Arguments;
};

#endif // CPUTPARSER_H

