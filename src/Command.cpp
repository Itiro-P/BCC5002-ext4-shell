#include "../include/Command.hpp"

short Command::info() {
    return 0;
}

short Command::cat(const std::vector<std::string>& args) {
    return 0;
}

short Command::attr(const std::vector<std::string>& args) {
    return 0;
}

short Command::cd(const std::vector<std::string>& args) {
    return 0;
}

short Command::ls() {
    return 0;
}

short Command::test_inode(const std::vector<std::string>& args) {
    return 0;
}

short Command::test_block(const std::vector<std::string>& args) {
    return 0;
}

short Command::cp(const std::vector<std::string>& args) {
    return 0;
}

short Command::pwd() {
    return 0;
}

short Command::touch(const std::vector<std::string>& args) {
    return 0;
}

short Command::mkdir(const std::vector<std::string>& args) {
    return 0;
}

short Command::rm(const std::vector<std::string>& args) {
    return 0;
}

short Command::rmdir(const std::vector<std::string>& args) {
    return 0;
}

short Command::rename(const std::vector<std::string>& args) {
    return 0;
}

short Command::exit() {
    return 0;
}

short Command::clear() {
    // Usamos CSI J 2 para limpar a tela e CSI H para mover o cursor para a posição inicial.
    std::print("\033[2J\033[H");
    return 0;
}