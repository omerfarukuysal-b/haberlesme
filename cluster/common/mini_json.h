#pragma once
#include <string>

namespace minijson
{

    // Very naive JSON value extractor. Supports:
    //   "key":"string"   OR   "key":123 / true / false
    // Returns empty string if not found.
    inline std::string get(const std::string &json, const std::string &key)
    {
        const std::string k = "\"" + key + "\"";
        auto pos = json.find(k);
        if (pos == std::string::npos)
            return "";
        pos = json.find(':', pos);
        if (pos == std::string::npos)
            return "";
        pos++;
        while (pos < json.size() && json[pos] == ' ')
            pos++;
        if (pos >= json.size())
            return "";

        if (json[pos] == '"')
        {
            pos++;
            auto end = json.find('"', pos);
            if (end == std::string::npos)
                return "";
            return json.substr(pos, end - pos);
        }
        auto end = json.find_first_of(",}", pos);
        if (end == std::string::npos)
            end = json.size();
        return json.substr(pos, end - pos);
    }
    // "telemetry":{ ... } bloğunu substring olarak döndürür (çok minimal)
    inline std::string get_object(const std::string &json, const std::string &key)
    {
        const std::string k = "\"" + key + "\"";
        auto pos = json.find(k);
        if (pos == std::string::npos)
            return "";
        pos = json.find(':', pos);
        if (pos == std::string::npos)
            return "";
        pos++;
        while (pos < json.size() && (json[pos] == ' '))
            pos++;
        if (pos >= json.size() || json[pos] != '{')
            return "";

        int depth = 0;
        size_t start = pos;
        for (size_t i = pos; i < json.size(); i++)
        {
            if (json[i] == '{')
                depth++;
            else if (json[i] == '}')
            {
                depth--;
                if (depth == 0)
                {
                    return json.substr(start, i - start + 1);
                }
            }
        }
        return "";
    }

} // namespace minijson
