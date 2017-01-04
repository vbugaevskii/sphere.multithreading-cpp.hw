#include "Parser.h"

void Parser::drop_string_to_buffer(std::string& s)
{
    if (s.size() > 0)
    {
        operands.push_back(s);
        s.clear();
    }
}

std::vector<std::string>& Parser::parse(std::string& command)
{
    operands.clear();
    std::string cmd = "", op = "", quote = "";

    for (char c : command)
    {
        if (quote.size() > 0)
        {
            quote += c;
            if (quote[0] == c)
                drop_string_to_buffer(quote);
        }
        else
        {
            if (isspace(c))
            {
                drop_string_to_buffer(cmd);
                drop_string_to_buffer(op);
            }
            else if (c == '<' || c == '>' || c == ';')
            {
                drop_string_to_buffer(cmd);
                drop_string_to_buffer(op);
                operands.push_back(std::string("") + c);
            }
            else if (c == '&' || c == '|')
            {
                drop_string_to_buffer(cmd);
                op += c;
            }
            else if (c == '\'' || c == '\"')
            {
                drop_string_to_buffer(cmd);
                drop_string_to_buffer(op);
                quote += c;
            }
            else
            {
                drop_string_to_buffer(op);
                cmd += c;
            }
        }
    }

    drop_string_to_buffer(cmd);
    drop_string_to_buffer(op);

    return operands;
}