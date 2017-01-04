#ifndef P4_PARSER_H
#define P4_PARSER_H

#include <string>
#include <vector>


class Parser {
    std::vector<std::string> operands;

public:
    std::vector<std::string>& parse(std::string& command);

private:
    void drop_string_to_buffer(std::string& s);
};

#endif //P4_PARSER_H
