#ifndef COMPILERERROR_H
#define COMPILERERROR_H

#include <stdexcept>

class CompilerError : public std::runtime_error {
public:
    enum class ErrorType {
      TypeError,
      NameError,
      SyntaxError,
      ReferenceError,
      Other
    };

    CompilerError(const std::string& message)
            :std::runtime_error(message), type(ErrorType::Other), line(-1), column(-1) { }

    CompilerError(ErrorType type, const std::string& message)
            :std::runtime_error(message), type(type), line(-1), column(-1) { }

    CompilerError(ErrorType type, const std::string& message,
            size_t line, size_t column)
            :std::runtime_error(message), type(type), line(line), column(column) { }

    ErrorType type;
    size_t line;
    size_t column;
};

#endif //COMPILERERROR_H
