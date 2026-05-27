#pragma once

#include <string>
#include <print>

/**
 * @brief Classe responsável por representar a interface de linha de comando do shell.
 */
class Shell {
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