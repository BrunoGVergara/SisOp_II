#include "Util.h"
#include <fstream>
#include <sstream>
#include <iostream>

// Função para ler o arquivo e armazenar as conexões
std::vector<Conexao> lerArquivo()
{
    std::vector<Conexao> conexoes;
    const std::string caminhoArquivo = "server.txt";
    std::ifstream arquivo(caminhoArquivo);
    if (!arquivo.is_open())
    {
        std::cerr << "Erro ao abrir o arquivo: " << std::endl;
        return conexoes;
    }

    std::string linha;
    while (std::getline(arquivo, linha))
    {
        std::istringstream iss(linha);
        std::string origem, destino;

        if (std::getline(iss, origem, '-') && iss.get() == '>' && std::getline(iss, destino))
        {
            conexoes.push_back({origem, destino});
        }
    }

    arquivo.close();
    return conexoes;
}

std::string buscarDestino(const std::string &origem)
{

    std::vector<Conexao> conexoes = lerArquivo();

    for (const auto &conexao : conexoes)
    {
        if (conexao.origem == origem)
        {
            return conexao.destino;
        }
    }

    return "Destino não encontrado";
}
