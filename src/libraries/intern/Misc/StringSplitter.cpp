#include "StringSplitter.h"

std::vector<std::string_view> StringViewSplitter(std::string_view input, char c)
{
    std::vector<std::string_view> results;

    int lastItemEnd = -1;

    for(int i = 0; i < static_cast<int>(input.size()); i++)
    {
        // itemEndIndex needed? or is it == i
        if(input[i] == c)
        {
            std::string_view item(input.data() + lastItemEnd + 1, i - lastItemEnd - 1);
            results.push_back(item);
            lastItemEnd = i;
        }
    }

    if(lastItemEnd != input.size())
    {
        results.emplace_back(input.data() + lastItemEnd + 1, input.size() - lastItemEnd - 1);
    }

    return results;
}