#pragma once

#include "Ext4Image.hpp"
#include <string>
#include <cstdint>
#include <vector>
#include <array>
#include <print>

namespace Command {
    /**
     * @brief Estrutura para armazenar informações sobre os comandos disponíveis no shell.
     */
    inline constexpr std::array<std::pair<std::string_view, std::string_view>, 17> command_info{{
        {"help",                            "Exibe este comando."},
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
        {"rename <arquivo> <novo nome>",    "Renomeia um arquivo ou diretório."},
        {"exit",                            "Encerra o shell."},
        {"clear",                           "Limpa a tela do shell."}
    }};

    /**
     * @brief Exibe a lista de comandos disponíveis.
     * @returns Um código de saída indicando o resultado da execução do comando. Normalmente, 0 para sucesso e um valor diferente de zero para erros.
     */
    short help();

    /**
     * @brief Exibe informações da imagem e do sistema de arquivos.
     * @param img A imagem do sistema de arquivos EXT4 montada, para acessar suas informações.
     * @returns Um código de saída indicando o resultado da execução do comando. Normalmente, 0 para sucesso e um valor diferente de zero para erros.
     */
    short info(Ext4Image& img);

    /**
     * @brief Exibe o conteúdo de um arquivo no formato texto.
     * @param img A imagem do sistema de arquivos EXT4 montada, para acessar suas informações.
     * @param args O vetor de argumentos, onde `args[1]` deve ser o caminho do arquivo a ser exibido.
     * @returns Um código de saída indicando o resultado da execução do comando. Normalmente, 0 para sucesso e um valor diferente de zero para erros.
     */
    short cat(Ext4Image& img, const std::vector<std::string>& args);

    /**
     * @brief Exibe os atributos de um arquivo ou diretório.
     * @param img A imagem do sistema de arquivos EXT4 montada, para acessar suas informações.
     * @param args O vetor de argumentos, onde `args[1]` deve ser o caminho do arquivo/diretório alvo.
     * @returns Um código de saída indicando o resultado da execução do comando. Normalmente, 0 para sucesso e um valor diferente de zero para erros.
     */
    short attr(Ext4Image& img, const std::vector<std::string>& args);

    /**
     * @brief Muda o diretório atual para o especificado.
     * @param img A imagem do sistema de arquivos EXT4 montada, para acessar suas informações.
     * @param args O vetor de argumentos, onde `args[1]` deve ser o caminho do diretório destino.
     * @returns Um código de saída indicando o resultado da execução do comando. Normalmente, 0 para sucesso e um valor diferente de zero para erros.
     */
    short cd(Ext4Image& img, const std::vector<std::string>& args);

    /**
     * @brief Lista o conteúdo do diretório atual.
     * @param img A imagem do sistema de arquivos EXT4 montada, para acessar suas informações.
     * @param args O vetor de argumentos, onde `args[1]` deve ser o caminho do diretório a ser visualizado.
     * Caso não seja fornecido um argumento, o conteúdo do diretório atual será listado.
     * @returns Um código de saída indicando o resultado da execução do comando. Normalmente, 0 para sucesso e um valor diferente de zero para erros.
     */
    short ls(Ext4Image& img, const std::vector<std::string>& args);

    /**
     * @brief Testa se um `inode` está livre ou ocupado.
     * @param img A imagem do sistema de arquivos EXT4 montada, para acessar suas informações.
     * @param args O vetor de argumentos, onde `args[1]` deve ser o número do inode (em formato string) a ser testado.
     * @returns Um código de saída indicando o resultado da execução do comando. Normalmente, 0 para sucesso e um valor diferente de zero para erros.
     */
    short test_inode(Ext4Image& img, const std::vector<std::string>& args);

    /**
     * @brief Testa se um `bloco` está livre ou ocupado.
     * @param img A imagem do sistema de arquivos EXT4 montada, para acessar suas informações.
     * @param args O vetor de argumentos, onde `args[1]` deve ser o número do bloco (em formato string) a ser testado.
     * @returns Um código de saída indicando o resultado da execução do comando. Normalmente, 0 para sucesso e um valor diferente de zero para erros.
     */
    short test_block(Ext4Image& img, const std::vector<std::string>& args);

    /**
     * @brief Copia um arquivo para outro local.
     * Na especificação do trabalho, o nome do comando deveria ser `export`. Mas, ao menos no `C++23`, `export` é uma palavra reservada, então o nome do comando foi alterado para `cp`.
     * @param img A imagem do sistema de arquivos EXT4 montada, para acessar suas informações.
     * @param args O vetor de argumentos, onde `args[1]` é o caminho do arquivo de origem e `args[2]` é o caminho do destino.
     * @returns Um código de saída indicando o resultado da execução do comando. Normalmente, 0 para sucesso e um valor diferente de zero para erros.
     */
    short cp(Ext4Image& img, const std::vector<std::string>& args);

    /**
     * @brief Exibe o diretório atual.
     * @param img A imagem do sistema de arquivos EXT4 montada, para acessar suas informações.
     * @returns Um código de saída indicando o resultado da execução do comando. Normalmente, 0 para sucesso e um valor diferente de zero para erros.
     */
    short pwd(Ext4Image& img);

    /**
     * @brief Cria um arquivo vazio.
     * @param img A imagem do sistema de arquivos EXT4 montada, para acessar suas informações.
     * @param args O vetor de argumentos, onde `args[1]` deve ser o caminho do arquivo a ser criado.
     * @returns Um código de saída indicando o resultado da execução do comando. Normalmente, 0 para sucesso e um valor diferente de zero para erros.
     */
    short touch(Ext4Image& img, const std::vector<std::string>& args);

    /**
     * @brief Cria um diretório.
     * @param img A imagem do sistema de arquivos EXT4 montada, para acessar suas informações.
     * @param args O vetor de argumentos, onde `args[1]` deve ser o caminho do diretório a ser criado.
     * @returns Um código de saída indicando o resultado da execução do comando. Normalmente, 0 para sucesso e um valor diferente de zero para erros.
     */
    short mkdir(Ext4Image& img, const std::vector<std::string>& args);

    /**
     * @brief Remove um arquivo.
     * @param img A imagem do sistema de arquivos EXT4 montada, para acessar suas informações.
     * @param args O vetor de argumentos, onde `args[1]` deve ser o caminho do arquivo a ser removido.
     * @returns Um código de saída indicando o resultado da execução do comando. Normalmente, 0 para sucesso e um valor diferente de zero para erros.
     */
    short rm(Ext4Image& img, const std::vector<std::string>& args);

    /**
     * @brief Remove um diretório vazio.
     * @param img A imagem do sistema de arquivos EXT4 montada, para acessar suas informações.
     * @param args O vetor de argumentos, onde `args[1]` deve ser o caminho do diretório a ser removido (que deve estar vazio).
     * @returns Um código de saída indicando o resultado da execução do comando. Normalmente, 0 para sucesso e um valor diferente de zero para erros.
     */
    short rmdir(Ext4Image& img, const std::vector<std::string>& args);

    /**
     * @brief Renomeia um arquivo.
     * @param img A imagem do sistema de arquivos EXT4 montada, para acessar suas informações.
     * @param args O vetor de argumentos, onde `args[1]` é o caminho atual do arquivo e `args[2]` é o novo nome/caminho.
     * @returns Um código de saída indicando o resultado da execução do comando. Normalmente, 0 para sucesso e um valor diferente de zero para erros.
     */
    short rename(Ext4Image& img, const std::vector<std::string>& args);

    /**
     * @brief Limpa o terminal.
     * @returns Um código de saída indicando o resultado da execução do comando. Normalmente, 0 para sucesso e um valor diferente de zero para erros.
     */
    short clear();
}