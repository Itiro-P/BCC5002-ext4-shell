#pragma once

#include <string>
#include <cstdint>
#include <vector>

namespace Command {
    /**
     * @brief Exibe informações da imagem e do sistema de arquivos.
     */
    void info();

    /**
     * @brief Exibe o conteúdo de um arquivo no formato texto.
     * 
     * @param file_path O caminho do arquivo a ser exibido.
     */
    void cat(const std::string& file_path);

    /**
     * @brief Exibe os atributos de um arquivo ou diretório.
     * 
     * @param path O caminho do arquivo ou diretório para o qual os atributos serão exibidos.
     */
    void attr(const std::string& path);

    /**
     * @brief Muda o diretório atual para o especificado.
     * 
     * @param path O caminho do diretório para o qual o diretório atual será mudado.
     */
    void cd(const std::string& path);

    /**
     * @brief Lista o conteúdo do diretório atual.
     */
    void ls();

    /**
     * @brief Testa se um `inode` está livre ou ocupado.
     * 
     * @param inode_number O número do inode a ser testado.
     */
    void test_inode(const uint32_t inode_number);

    /**
     * @brief Testa se um `bloco` está livre ou ocupado.
     * 
     * @param block_number O número do bloco a ser testado.
     */
    void test_block(const uint32_t block_number);

    /**
     * @brief Copia um arquivo para outro local.
     * 
     * Na especificação do trabalho, o nome do comando deveria ser `export`. Mas, ao menos no `C++23`, `export` é uma palavra reservada, então o nome do comando foi alterado para `cp`.
     * 
     * @param source_path O caminho do arquivo de origem.
     * @param target_path O caminho do arquivo de destino.
     */
    void cp(const std::string& source_path, const std::string& target_path);
    
    /**
     * @brief Exibe o diretório atual.
     */
    void pwd();

    /**
     * @brief Cria um arquivo vazio.
     * 
     * @param file_path O caminho do arquivo a ser criado.
     */
    void touch(const std::string& file_path);

    /**
     * @brief Cria um diretório.
     * 
     * @param path O caminho do diretório a ser criado.
     */
    void mkdir(const std::string& path);

    /**
     * @brief Remove um arquivo.
     * 
     * @param file_path O caminho do arquivo a ser removido.
     */
    void rm(const std::string& file_path);

    /**
     * @brief Remove um diretório vazio.
     * 
     * @param path O caminho do diretório a ser removido. Deve estar vazio.
     */
    void rmdir(const std::string& path);

    /**
     * @brief Renomeia um arquivo.
     * 
     * @param file O caminho do arquivo a ser renomeado.
     * @param new_file_name O novo nome do arquivo.
     */
    void rename(const std::string& file, const std::string& new_file_name);

    /**
     * @brief Sai do programa.
     */
    void exit();
}