#pragma once

#include "Command.hpp"
#include "Ext4Image.hpp"
#include <string>
#include <print>
#include <unordered_map>
#include <unordered_set>
#include <functional>


typedef std::vector<std::string> CommandArgs;

/**
 * @brief Classe responsável por representar a interface de linha de comando do shell.
 */
class Shell {
    // Imagem que será carregada e manipulada pelos comandos do shell.
    Ext4Image image;

    // Comandos para saida do shell.
    static inline std::unordered_set<std::string> exit_commands{"exit", "quit", "\\q"};
    // Hash de comandos com suas respectivas funções.
    static inline std::unordered_map<std::string, std::function<short(Ext4Image&, const CommandArgs&)>> command_map{ 
        {"help",   [](Ext4Image& img, const CommandArgs& args) { return Command::help(); }},
        {"info",   [](Ext4Image& img, const CommandArgs& args) { return Command::info(img); }},
        {"cat",    [](Ext4Image& img, const CommandArgs& args) { return Command::cat(img, args); }},
        {"attr",   [](Ext4Image& img, const CommandArgs& args) { return Command::attr(img, args); }},
        {"cd",     [](Ext4Image& img, const CommandArgs& args) { return Command::cd(img, args); }},
        {"ls",     [](Ext4Image& img, const CommandArgs& args) { return Command::ls(img, args); }},
        {"testi",  [](Ext4Image& img, const CommandArgs& args) { return Command::test_inode(img, args); }},
        {"testb",  [](Ext4Image& img, const CommandArgs& args) { return Command::test_block(img, args); }},
        {"cp",     [](Ext4Image& img, const CommandArgs& args) { return Command::cp(img, args); }},
        {"export", [](Ext4Image& img, const CommandArgs& args) { return Command::cp(img, args); }},
        {"pwd",    [](Ext4Image& img, const CommandArgs& args) { return Command::pwd(img); }},
        {"touch",  [](Ext4Image& img, const CommandArgs& args) { return Command::touch(img, args); }},
        {"mkdir",  [](Ext4Image& img, const CommandArgs& args) { return Command::mkdir(img, args); }},
        {"rm",     [](Ext4Image& img, const CommandArgs& args) { return Command::rm(img, args); }},
        {"rmdir",  [](Ext4Image& img, const CommandArgs& args) { return Command::rmdir(img, args); }},
        {"rename", [](Ext4Image& img, const CommandArgs& args) { return Command::rename(img, args); }},
        {"clear",  [](Ext4Image& img, const CommandArgs& args) { return Command::clear(); }}
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
     * @brief Construtor do shell, que recebe o caminho da imagem do sistema de arquivos a ser montada.
     * @param image_path O caminho da imagem do sistema de arquivos a ser montada.
     */
    Shell(const std::string& image_path): image(image_path) {}

    /**
     * @brief Roda o shell, aguardando por comandos do usuário.
     * 
     * @returns Um código de saída indicando o resultado da execução do shell. Normalmente, 0 para sucesso e um valor diferente de zero para erros.
     */
    short run();
};