#pragma once

#include <string>
#include <print>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include "Command.hpp"

// Lembrete: 
typedef std::vector<std::string> CommandArgs;

/**
 * @brief Classe responsável por representar a interface de linha de comando do shell.
 */
class Shell {
    // Comandos para saida do shell.
    static inline std::unordered_set<std::string> exit_commands{"exit", "quit", "\\q"};
    // Hash de comandos com suas respectivas funções.
    static inline std::unordered_map<std::string, std::function<short(const CommandArgs&)>> command_map{ 
        {"info",       [](const CommandArgs& args) { return Command::info(); }},
        {"cat",        [](const CommandArgs& args) { return Command::cat(args); }},
        {"attr",       [](const CommandArgs& args) { return Command::attr(args); }},
        {"cd",         [](const CommandArgs& args) { return Command::cd(args); }},
        {"ls",         [](const CommandArgs& args) { return Command::ls(args); }},
        {"testi",      [](const CommandArgs& args) { return Command::test_inode(args); }},
        {"testb",      [](const CommandArgs& args) { return Command::test_block(args); }},
        {"cp",         [](const CommandArgs& args) { return Command::cp(args); }},
        {"export",     [](const CommandArgs& args) { return Command::cp(args); }},
        {"pwd",        [](const CommandArgs& args) { return Command::pwd(); }},
        {"touch",      [](const CommandArgs& args) { return Command::touch(args); }},
        {"mkdir",      [](const CommandArgs& args) { return Command::mkdir(args); }},
        {"rm",         [](const CommandArgs& args) { return Command::rm(args); }},
        {"rmdir",      [](const CommandArgs& args) { return Command::rmdir(args); }},
        {"rename",     [](const CommandArgs& args) { return Command::rename(args); }},
        {"clear",      [](const CommandArgs& args) { return Command::clear(); }}
    };

    /** 
     * @brief Imprime o prompt do shell. 
     * @param current_directory O diretório atual a ser exibido no prompt.
    */
    void print_prompt(const std::string& current_directory) {
        std::print("({})> ", current_directory);
    }

public:
    /**
     * @brief Inicia o shell, aguardando por comandos do usuário.
     * 
     * @param image_path O caminho da imagem do sistema de arquivos a ser montada.
     * 
     * @returns Um código de saída indicando o resultado da execução do shell. Normalmente, 0 para sucesso e um valor diferente de zero para erros.
     */
    short run(const std::string& image_path);
};