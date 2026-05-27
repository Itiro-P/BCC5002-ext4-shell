#include "../include/Command.hpp"

std::vector<std::pair<std::string, std::string>> Command::command_info{
    {"info",                            "Exibe informações da imagem e do sistema de arquivos."},
    {"cat <arquivo>",                   "Exibe o conteúdo de um arquivo no formato texto."},
    {"attr <arquivo/diretório>",        "Exibe os atributos de um arquivo ou diretório."},
    {"cd <diretório>",                  "Muda o diretório atual para o especificado."},
    {"ls <diretório>",                  "Lista o conteúdo do diretório atual ou o especificado (caso <diretório> seja fornecido)."},
    {"test_inode <inode>",              "Testa se um `inode` está livre ou ocupado."},
    {"test_block <bloco>",              "Testa se um `bloco` está livre ou ocupado."},
    {"cp/export <arquivo1> <arquivo2>", "Copia um arquivo para outro local."},
    {"pwd",                             "Exibe o caminho do diretório atual."},
    {"touch <arquivo>",                 "Cria um novo arquivo vazio ou atualiza a data de modificação de um arquivo existente."},
    {"mkdir <diretório>",               "Cria um novo diretório."},
    {"rm <arquivo>",                    "Remove um arquivo."},
    {"rmdir <diretório>",               "Remove um diretório vazio."},
    {"rename <antigo> <novo>",          "Renomeia um arquivo ou diretório."},
    {"exit",                            "Encerra o shell."},
    {"clear",                           "Limpa a tela do terminal."}
};

short Command::info() {
    for(const auto& [cmd, desc] : Command::command_info) {
        std::println("- {:<32} - {}", cmd, desc);
    }
    return 0;
}

short Command::cat(const std::vector<std::string>& args) {
    const std::string file_path = args.size() > 1 ? args[1] : "";
    return 0;
}

short Command::attr(const std::vector<std::string>& args) {
    return 0;
}

short Command::cd(const std::vector<std::string>& args) {
    return 0;
}

short Command::ls(const std::vector<std::string>& args) {
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