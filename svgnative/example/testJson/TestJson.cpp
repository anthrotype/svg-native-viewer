/*
Copyright 2019 Adobe. All rights reserved.
This file is licensed to you under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License. You may obtain a copy
of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under
the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR REPRESENTATIONS
OF ANY KIND, either express or implied. See the License for the specific language
governing permissions and limitations under the License.
*/

#include "SVGDocument.h"
#include "JsonSVGRenderer.h"

#include <fstream>
#include <iostream>
#include <string>

int main(int argc, char* const argv[])
{
    if (argc != 3 && argc != 4)
    {
        std::cout << "Incorrect number of arguments." << std::endl;
        return 0;
    }

    std::string svgInput{};
    std::ifstream input(argv[1]);
    if (!input)
    {
        std::cout << "Error! Could not open input file." << std::endl;
        exit(EXIT_FAILURE);
    }
    for (std::string line; std::getline(input, line);)
        svgInput.append(line);
    input.close();

    SVGNative::ColorMap colorMap = {
        {"test-red",   {{0.502,   0.0, 0.0, 1.0}}},
        {"test-green", {{  0.0, 0.502, 0.0, 1.0}}},
        {"test-blue",  {{  0.0,   0.0, 1.0, 1.0}}}
    };
    auto renderer = std::make_shared<SVGNative::JsonSVGRenderer>();

    auto doc = std::unique_ptr<SVGNative::SVGDocument>(SVGNative::SVGDocument::CreateSVGDocument(svgInput.c_str(), renderer));
    if (argc == 3)
        doc->Render(colorMap);
    else
    {
        std::string id{argv[3]};
        doc->Render(id.c_str(), colorMap);
    }

    std::fstream outputStream;
    outputStream.open(argv[2], std::fstream::out);
    if (!outputStream)
    {
        std::cout << "Error! Could not write file." << std::endl;
        exit(EXIT_FAILURE);
    }
    std::string outputString{renderer->Json().dump(2)};
    outputStream << outputString;
    outputStream.close();

    return 0;
}
