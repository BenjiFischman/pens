#include "verification_code.hpp"
#include "logger.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace Pens {

VerificationCodeGenerator::VerificationCodeGenerator()
    : rd_(),
      gen_(rd_()),
      dist_(100000, 999999) {  // 6-digit range
    LOG_DEBUG("Verification code generator initialized");
}

std::string VerificationCodeGenerator::generate() {
    int code = dist_(gen_);
    
    std::ostringstream oss;
    oss << std::setw(6) << std::setfill('0') << code;
    
    std::string codeStr = oss.str();
    LOG_INFO("Generated verification code: " + codeStr.substr(0, 3) + "***");
    
    return codeStr;
}

bool VerificationCodeGenerator::validate(const std::string& code) {
    if (code.length() != 6) {
        return false;
    }
    
    return std::all_of(code.begin(), code.end(), ::isdigit);
}

} // namespace Pens

