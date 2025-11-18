#include <iostream>
#include <fstream>

#include "ast.hpp"
#include "json.hpp"
#include "builder.hpp"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <input.astj>." << std::endl;
        return 1;
    }

    std::ifstream inputFile(argv[1]);
    
    if (!inputFile.is_open()) {
        std::cerr << "Could not open file " << argv[1] << "." << std::endl;
        return 1;
    }

    nlohmann::json json;

    try {
       inputFile >> json; 
    } catch (const nlohmann::json::parse_error &e) {
        std::cerr << "JSON parsing error " << e.what() << std::endl;
        return 1;
    }

    try {
        std::unique_ptr<Program> program = buildProgram(json);
        program->check();
        std::cout << "valid" << std::endl;
    } catch (const std::runtime_error &e) {
        std::cout << "invalid: " << e.what() << std::endl;  
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    } 

    return 0;
}
