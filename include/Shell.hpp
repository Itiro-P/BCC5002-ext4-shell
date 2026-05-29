#pragma once

#include "Command.hpp"
#include "Ext4.hpp"
#include <string>
#include <print>
#include <unordered_map>
#include <unordered_set>
#include <functional>


typedef std::vector<std::string> CommandArgs;

/**
 * @brief Classe responsável por representar a interface de linha de comando do shell.
 * @author Pedro Itiro Nagao
 */
class Shell {
    // Imagem que será carregada e manipulada pelos comandos do shell.
    Ext4::Image image;

    // Comandos para saida do shell.
    static inline std::unordered_set<std::string> exit_commands{"exit", "quit", "\\q"};
    // Hash de comandos com suas respectivas funções.
    static inline std::unordered_map<std::string, std::function<short(Ext4::Image&, const CommandArgs&)>> command_map{ 
        {"help",   [](Ext4::Image& img, const CommandArgs& args) { return Command::help(); }},
        {"info",   [](Ext4::Image& img, const CommandArgs& args) { return Command::info(img); }},
        {"cat",    [](Ext4::Image& img, const CommandArgs& args) { return Command::cat(img, args); }},
        {"attr",   [](Ext4::Image& img, const CommandArgs& args) { return Command::attr(img, args); }},
        {"cd",     [](Ext4::Image& img, const CommandArgs& args) { return Command::cd(img, args); }},
        {"ls",     [](Ext4::Image& img, const CommandArgs& args) { return Command::ls(img, args); }},
        {"testi",  [](Ext4::Image& img, const CommandArgs& args) { return Command::test_inode(img, args); }},
        {"testb",  [](Ext4::Image& img, const CommandArgs& args) { return Command::test_block(img, args); }},
        {"cp",     [](Ext4::Image& img, const CommandArgs& args) { return Command::cp(img, args); }},
        {"export", [](Ext4::Image& img, const CommandArgs& args) { return Command::cp(img, args); }},
        {"pwd",    [](Ext4::Image& img, const CommandArgs& args) { return Command::pwd(img); }},
        {"touch",  [](Ext4::Image& img, const CommandArgs& args) { return Command::touch(img, args); }},
        {"mkdir",  [](Ext4::Image& img, const CommandArgs& args) { return Command::mkdir(img, args); }},
        {"rm",     [](Ext4::Image& img, const CommandArgs& args) { return Command::rm(img, args); }},
        {"rmdir",  [](Ext4::Image& img, const CommandArgs& args) { return Command::rmdir(img, args); }},
        {"rename", [](Ext4::Image& img, const CommandArgs& args) { return Command::rename(img, args); }},
        {"clear",  [](Ext4::Image& img, const CommandArgs& args) { return Command::clear(); }}
    };

/** * @brief Imprime o prompt do shell com base no tipo do inode atual.
 */
void print_prompt() {
    Ext4::InodeMode type = this->image.get_current_inode().get_type();
    std::string_view type_str = "unknown";

    switch (type) {
        case Ext4::InodeMode::S_IFIFO:
            type_str = "fifo";
            break;
        case Ext4::InodeMode::S_IFCHR:
            type_str = "chr";
            break;
        case Ext4::InodeMode::S_IFDIR:
            type_str = "dir";
            break;
        case Ext4::InodeMode::S_IFBLK:
            type_str = "blk";
            break;
        case Ext4::InodeMode::S_IFREG:
            type_str = "file";
            break;
        case Ext4::InodeMode::S_IFLNK:
            type_str = "lnk";
            break;
        case Ext4::InodeMode::S_IFSOCK:
            type_str = "sock";
            break;
        default:
            type_str = "unknown";
            break;
    }

    std::print("({})> ", type_str);
}

public:
    /**
     * @brief Construtor do shell, que recebe o caminho da imagem do sistema de arquivos a ser montada.
     * @param image_path O caminho da imagem do sistema de arquivos a ser montada.
     */
    Shell(const std::string& image_path): image(image_path) {}

    /**
     * @brief Roda o shell, aguardando por comandos do usuário.
     * @returns Um código de saída indicando o resultado da execução do shell. Normalmente, 0 para sucesso e um valor diferente de zero para erros.
     */
    short run();
};