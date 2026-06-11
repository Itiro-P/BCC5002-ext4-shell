# BCC5002-ext4-shell

Este é o código para o projeto final da disciplina de Sistemas Operacionais (5002). O objetivo é manipular manualmente imagens em *ext4* usando a linguagem de programação `C++`.

**Estrutura mínima do repositório**

- `main.cpp` — código-fonte principal.
- `Makefile` — regras de compilação.
- `LICENSE` — licença do projeto.

**Requisitos**

- Ambiente Linux (testado em distribuições modernas).
- `g++` com suporte a C++23 (ex.: GCC 11+).
- ferramentas básicas: `make`, `rm`.

**Como compilar**

Opções recomendadas:

- Usando o `Makefile` (recomendado):

	- Compilação release (otimizada):

		```bash
		make
		```

		Isso gera o binário `ext4shell`.

	- Compilação para depuração (AddressSanitizer, símbolos de depuração):

		```bash
		make debug
		```

	- Limpar artefatos de build:

		```bash
		make clean
		```

- Compilar diretamente com `g++` (sem `make`):

	```bash
	g++ main.cpp -o ext4shell -std=c++23 -Wall -Wextra -pedantic -O2
	```

	Para build de debug equivalente ao `make debug`:

	```bash
	g++ main.cpp -o ext4shell -std=c++23 -Wall -Wextra -pedantic -g -O0 -DDEBUG -fsanitize=address,undefined
	```

**Como executar**

Após compilar, execute:

```bash
./ext4shell
```

**Notas**

- Se ocorrerem erros relacionados ao padrão C++ (`-std=c++23`), verifique a versão do GCC (`g++ --version`) e atualize se necessário.
- O alvo padrão do `Makefile` produz uma versão otimizada; use `make debug` para diagnóstico com sanitizers.
