#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <vector>

struct Conexao
{
    std::string origem;
    std::string destino;
};

std::vector<Conexao> lerArquivo();
std::string buscarDestino(const std::string &origem);

#endif // UTIL_H