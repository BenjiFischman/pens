#ifndef VERIFICATION_CODE_HPP
#define VERIFICATION_CODE_HPP

#include <string>
#include <random>

namespace Pens {

/**
 * @brief Verification Code Generator
 * 
 * Generates secure 6-digit verification codes for email verification
 */
class VerificationCodeGenerator {
public:
    VerificationCodeGenerator();
    
    /**
     * Generate a random 6-digit verification code
     * @return String containing 6 digits (000000-999999)
     */
    std::string generate();
    
    /**
     * Validate a verification code format
     * @param code The code to validate
     * @return true if code is 6 digits
     */
    bool validate(const std::string& code);
    
private:
    std::random_device rd_;
    std::mt19937 gen_;
    std::uniform_int_distribution<int> dist_;
};

} // namespace Pens

#endif // VERIFICATION_CODE_HPP

