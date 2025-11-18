#pragma once
#include <stdexcept>
#include <string>

namespace gs_patterns
{
    class GSError : public std::exception
    {
    public:
        explicit GSError (std::string  reason) : _reason(std::move(reason)) { }
        ~GSError() override = default;

        [[nodiscard]] const char * what() const noexcept override { return _reason.c_str(); }
    private:
        std::string _reason;
    };

    class GSFileError : public GSError
    {
    public:
        explicit GSFileError (std::string reason) : GSError(std::move(reason)) { }
        ~GSFileError() override = default;
    };

    class GSDataError : public GSError
    {
    public:
        explicit GSDataError (std::string reason) : GSError(std::move(reason)) { }
        ~GSDataError() override = default;
    };

    class GSAllocError : public GSError
    {
    public:
        explicit GSAllocError (std::string reason) : GSError(std::move(reason)) { }
        ~GSAllocError() override = default;
    };
}