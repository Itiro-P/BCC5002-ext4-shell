#pragma once

#include <string>
#include <print>
#include <unordered_map>
#include <functional>
#include "Command.hpp"

// Lembrete: 
typedef std::vector<std::string> CommandArgs;

/**
 * @brief Classe responsável por representar a interface de linha de comando do shell.
 */
class Shell {
    // Hash de comandos com suas respectivas funções.
    std::unordered_map<std::string, std::function<short(const CommandArgs&)>> command_map{
        {"info",       [](const CommandArgs& args) { return Command::info(); }},
        {"cat",        [](const CommandArgs& args) { return Command::cat(args[1]); }},
        {"attr",       [](const CommandArgs& args) { return Command::attr(args[1]); }},
        {"cd",         [](const CommandArgs& args) { return Command::cd(args[1]); }},
        {"ls",         [](const CommandArgs& args) { return Command::ls(); }},
        {"test_inode", [](const CommandArgs& args) { return Command::test_inode(std::stoul(args[1])); }},
        {"test_block", [](const CommandArgs& args) { return Command::test_block(std::stoul(args[1])); }},
        {"cp",         [](const CommandArgs& args) { return Command::cp(args[1], args[2]); }},
        {"export",     [](const CommandArgs& args) { return Command::cp(args[1], args[2]); }},
        {"pwd",        [](const CommandArgs& args) { return Command::pwd(); }},
        {"touch",      [](const CommandArgs& args) { return Command::touch(args[1]); }},
        {"mkdir",      [](const CommandArgs& args) { return Command::mkdir(args[1]); }},
        {"rm",         [](const CommandArgs& args) { return Command::rm(args[1]); }},
        {"rmdir",      [](const CommandArgs& args) { return Command::rmdir(args[1]); }},
        {"rename",     [](const CommandArgs& args) { return Command::rename(args[1], args[2]); }},
        {"exit",       [](const CommandArgs& args) { return Command::exit(); }}
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