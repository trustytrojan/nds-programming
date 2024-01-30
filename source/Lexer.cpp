#include "Lexer.hpp"

void EscapeInUnquotedString(StringIterator &itr, std::string &currentToken)
{
	// `itr` is pointing at the initial backslash.
	// Only escape spaces, backslashes, and equals.
	// Otherwise, omit the backslash.
	switch (*(++itr))
	{
	case ' ':
		currentToken += ' ';
		break;
	case '=':
		currentToken += '=';
		break;
	case '\\':
		currentToken += '\\';
		break;
	default:
		currentToken += *itr;
	}
}

void EscapeInDoubleQuoteString(StringIterator &itr, std::string &currentToken)
{
	// `itr` is pointing at the initial backslash.
	// Only escape backslashes, double-quotes, and dollar signs.
	switch (*(++itr))
	{
	case '"':
		currentToken += '"';
		break;
	case '\\':
		currentToken += '\\';
		break;
	case '$':
		currentToken += '$';
		break;
	default:
		currentToken += *itr;
	}
}

void InsertVariable(StringIterator &itr, const StringIterator &lineEnd, std::string &currentToken, EnvMap &env)
{
	// `itr` is pointing at the initial `$`
	// subtitute variables in the lexing phase to avoid the reiteration overhead during parsing
	std::string varname;
	for (++itr; (isalnum(*itr) || *itr == '_') && *itr != '"' && itr < lineEnd; ++itr)
		varname += *itr;
	currentToken += env[varname];
	--itr; // Let the caller's loop see the character that stopped our loop, they might need it
}

bool LexDoubleQuoteString(StringIterator &itr, const StringIterator &lineEnd, std::string &currentToken, EnvMap &env)
{
	// When called, itr is pointing at the opening `"`, so increment before looping.
	for (++itr; *itr != '"' && itr < lineEnd; ++itr)
		switch (*itr)
		{
		case '\\':
			EscapeInDoubleQuoteString(itr, currentToken);
			break;

		case '$':
			InsertVariable(itr, lineEnd, currentToken, env);
			break;

		default:
			currentToken += *itr;
		}

	if (itr == lineEnd)
	{
		std::cerr << "\e[41mshell: closing `\"` not found\e[39m\n";
		return false;
	}

	return true;
}

bool LexSingleQuoteString(StringIterator &itr, const StringIterator &lineEnd, std::string &currentToken)
{
	// Nothing can be escaped in single-quote strings.
	for (++itr; *itr != '\'' && itr < lineEnd; ++itr)
		currentToken += *itr;

	if (itr == lineEnd)
	{
		std::cerr << "\e[41mshell: closing `'` not found\e[39m\n";
		return false;
	}

	return true;
}

bool ProcessAssignment(StringIterator &itr, const StringIterator &lineEnd, std::string key, EnvMap &env)
{
	// `currentToken` is copied into `key`
	// we need to build `value`
	std::string value;

	// `itr` is pointing at `=`, so increment ahead and continue
	// in bash assignments can look like `A="hello $X world"`, so we need to account for quoted strings as values
	for (++itr; !isspace(*itr) && itr < lineEnd; ++itr)
		switch (*itr)
		{
		case '"':
			// good thing these functions simply append to their `currentToken` parameter!
			// we can make use of this and make them append to `value`
			if (!LexDoubleQuoteString(itr, lineEnd, value, env))
				return false;
			break;

		case '\'':
			if (!LexSingleQuoteString(itr, lineEnd, value))
				return false;
			break;

		default:
			value += *itr;
		}

	// DEBUG
	std::cout << "=(" << key << ',' << value << ")\n";

	env[key] = value;
	return true;
}

bool LexLine(std::vector<Token> &tokens, const std::string &line, EnvMap &env)
{
	const auto lineEnd = line.cend();
	std::string currentToken;

	const auto pushAndClear = [&currentToken, &tokens]()
		{
			tokens.push_back({Token::Type::STRING, currentToken});
			currentToken.clear();
		};

	const auto pushAndClearIfNotEmpty = [&currentToken, &pushAndClear]()
		{
			if (!currentToken.empty())
				pushAndClear();
		};

	for (auto itr = line.cbegin(); itr < lineEnd; ++itr)
	{
		if (isspace(*itr) && !currentToken.empty())
		{
			pushAndClear();
			tokens.push_back({Token::Type::WHITESPACE});
			continue;
		}

		switch (*itr)
		{
		case '\\':
			EscapeInUnquotedString(itr, currentToken);
			break;

		case '\'':
			if (!LexSingleQuoteString(itr, lineEnd, currentToken))
				return false;
			break;

		case '"':
			if (!LexDoubleQuoteString(itr, lineEnd, currentToken, env))
				return false;
			break;

		case '=':
			// Validate that `currentToken` contains a valid env key name
			if (!std::ranges::all_of(currentToken, [](const char c) {return isalnum(c) || c == '_';}))
			{
				currentToken += '=';
				break;
			}
			if (!ProcessAssignment(itr, lineEnd, currentToken, env))
				return false;
			currentToken.clear();
			break;

		case '$':
			// bash treats $1, $2, etc as special variables, we won't for now.
			InsertVariable(itr, lineEnd, currentToken, env);
			break;

		case ';':
			pushAndClearIfNotEmpty();
			tokens.push_back({Token::Type::SEMICOLON});
			break;

		case '<':
			pushAndClearIfNotEmpty();
			tokens.push_back({Token::Type::INPUT_REDIRECT});
			break;

		case '>':
			pushAndClearIfNotEmpty();
			tokens.push_back({Token::Type::OUTPUT_REDIRECT});
			break;

		case '|':
			pushAndClearIfNotEmpty();

			if (*(++itr) == '|')
				tokens.push_back({Token::Type::OR});
			else
			{
				tokens.push_back({Token::Type::PIPE});
				--itr;
			}

			break;

		case '&':
			pushAndClearIfNotEmpty();

			if (*(++itr) == '&')
				tokens.push_back({Token::Type::AND});
			else
			{
				tokens.push_back({Token::Type::AMPERSAND});
				--itr;
			}

			break;

		default:
			currentToken += *itr;
		}
	}

	if (!currentToken.empty())
		tokens.push_back({Token::Type::STRING, currentToken});

	return true;
}
