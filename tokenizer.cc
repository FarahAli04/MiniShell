#include "tokenizer.h"
#include <sstream>
#include <iostream>

std::vector<Token> tokenize(const std::string &input)
{
    std::vector<Token> tokens;
    size_t i = 0;
    while (true)
    {	
	while(i < input.size() && isspace((unsigned char)input[i])) i++;
	if (i >= input.size()) break;
	std::string word;
	bool isQuotes = false;
	while (i < input.size() && (isQuotes || ! isspace((unsigned char)input[i])))
	{
	if (input[i] == '"') isQuotes = !isQuotes; 
        else word += input[i];
        i++;
	}
        Token token;

        if (word == "|")
            token.type = TOKEN_PIPE;
        else if (word == ">")
            token.type = TOKEN_REDIRECT;
        else if (word == ">>")
            token.type = TOKEN_APPEND;
        else if (word == "<")
            token.type = TOKEN_INPUT;
        else if (word == "&")
            token.type = TOKEN_BACKGROUND;
        else if (word == "2>")
            token.type = TOKEN_ERROR;
        else if (word == ">>&")
            token.type = TOKEN_REDIRECT_AND_ERROR;
        else
            {
                if (!tokens.empty())
                {
                    Token &prev = tokens.back();
                    if (prev.type == TOKEN_REDIRECT ||
                        prev.type == TOKEN_APPEND ||
                        prev.type == TOKEN_INPUT ||
                        prev.type == TOKEN_ERROR || 
                        prev.type == TOKEN_REDIRECT_AND_ERROR ||
                        prev.type == TOKEN_COMMAND || 
                        prev.type == TOKEN_ARGUMENT || 
                        word[0] == '-' ||
                        word.find('/') != std::string::npos ||
                        word.find('.') != std::string::npos

                    )
                    {
                        token.type = TOKEN_ARGUMENT;
                    }
                    else
                    {
                        token.type = TOKEN_COMMAND;
                    }
                }
                else
                {
                    token.type = TOKEN_COMMAND;
                }
            }

        token.value = word;
        tokens.push_back(token);
    }

    tokens.push_back({TOKEN_EOF, ""});
    return tokens;
}
