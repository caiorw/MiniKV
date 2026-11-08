Bancos de dados como PostgreSQL, MySQL e Redis atendem milhares de clientes ao mesmo tempo. Para isso, precisam resolver três problemas clássicos de programação concorrente:

1. Permitir que muitos clientes acessem os dados em paralelo sem corromper nada;
2. Garantir que uma operação confirmada ao cliente não se perca se a máquina desligar logo depois;
3. Conseguir tirar uma cópia consistente dos dados (um snapshot) sem parar o banco por muito tempo.

Neste trabalho, você vai implementar a sincronização de um pequeno banco chave-valor em memória, o MiniKV, que guarda saldos de contas bancárias. A estrutura de dados (a tabela hash), a escrita em disco, os clientes e a verificação de corretude já estão prontos no código base. Seu trabalho é fazer tudo isso funcionar corretamente com muitas threads.
