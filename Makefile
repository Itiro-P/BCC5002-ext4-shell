CXX      := g++
CXXFLAGS := -std=c++23 -Wall -Wextra -pedantic -O2
DBGFLAGS := -std=c++23 -Wall -Wextra -pedantic -g -O0 -DDEBUG -fsanitize=address,undefined

# A partir C++20 variáveis declaradas com designated initializers já são construídas com valores padrão
# caso uma propriedade seja omitida (fallback para valor padrão da struct ou construtor padrão).
# Mas, bizarramente, o compilador reclama. Então só suprimimos.
CXXFLAGS += -Wno-missing-field-initializers

TARGET   := ext4shell
SRCS     := main.cpp $(wildcard src/*.cpp) $(wildcard src/Wrappers/*.cpp)
OBJS     := $(SRCS:.cpp=.o)
DEPS     := $(SRCS:.cpp=.d)

.PHONY: all
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^
	@echo "  LD  $@"

%.o: %.cpp
	@echo "  CXX $<"
	$(CXX) $(CXXFLAGS) -MMD -MP -c -o $@ $<

# Inclui dependências geradas automaticamente (.d)
-include $(DEPS)

.PHONY: debug
debug: CXXFLAGS := $(DBGFLAGS)
debug: clean $(TARGET)
	@echo "  >> build de debug com ASan/UBSan"

.PHONY: clean
clean:
	@echo "  RM  objetos e binário"
	@rm -f $(OBJS) $(DEPS) $(TARGET)


.PHONY: help
help:
	@echo ""
	@echo "Alvos disponíveis:"
	@echo "  make               -> compila o projeto (release)"
	@echo "  make debug         -> compila com -g e AddressSanitizer"
	@echo "  make clean         -> remove objetos e binário"
	@echo "  make help          -> este texto"
	@echo ""