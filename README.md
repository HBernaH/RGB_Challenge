## Relatório

#### Funcionamento
O Cliente inicializa, verificando a ligação a um servidor válido. Caso exista pede  a resolução (válida até 8k) ao user, cria os 3 ficheiros (vermelho,verde e azul) em binário na sua subdirectoria files e começa a enviar ao servidor, ciclicamente, até o user  premir ctrl + C , estando o cliente capaz de enviar um pedido de término ao servidor, libertando este para outros clientes.
O Servidor, fica constantemente à escuta de datagramas, permitindo receber apenas de um cliente, descartando quaisquer outros. Dependendo do tipo de datagrama que recebe, descritos nas funções da struct packet  executa uma ação diferente:

#### Packet Types
- 0 → Connect: Reenvia um Connect de volta ao cliente 
- 1 → Disconnect: Ao receber, liberta para um novo cliente
- 2 → Resolution: Reenvia este para validação 
- 3 → File Ack: Reenvia para certificar que recebeu
- 4 → File data: Conteúdos do ficheiro
- 5 → Starting File data: Para escrever novo ficheiro na sua subdiretoria files
- 6 → Finishing File data: Para enviar uma confirmação posteriormente ao cliente, e receção de novo ficheiro

#### Diagrama
Cliente  -_-_-_-_-_-_-  Servidor

Connect —————⟶

   ⟵————— Connect ACK
		   
Resolution —————⟶ 

   ⟵—————-Resolution ACK
		   
File Begin —————⟶ 

File Frags —————⟶ 

...

File End   —————⟶ 

   ⟵—————- File End ACK

(Cliente Ctrl + C - SIGINT)
Disconnect-———⟶

#### Estruturação
Formato dos ficheiros
Inicialmente recorrendo a .PPM (Portable Pixel Map) em "Plain Text", devido ao tamanho elevado, recorri à escrita por binário, resultando em ficheiros 4x mais pequenos, sendo melhor depois para a verificação Server side, uma vez que o calculo deste (em bytes) seria apenas 3 x width x height.
Para uma boa separação e estruturação do código separei o projeto em três diretorias:

#### Lib/
Todos os ficheiros de apoio ao cliente e servidor, onde se encontram:
	file_tools.cpp/.hpp → Funções de apoio à criação dos ficheiros pretendidos. 
		File:
		Classe que possui todos os métodos para criação, do ficheiro, possuindo todos os atributos necessários.
	datagram.cpp/.hpp →  Toda a lógica para a criação do datagrama UDP.
		Packet:
		Estrutura que possui todos os métodos para a criação e atualização do payload dos datagramas, incluindo a fragmentação dos objetos File para envio, serialização e deserialização.
		Decidi limitar o payload do datagrama até 512 bytes, baseando-me no MTU mínimo em rede.
			Type (1 byte)  | Size (2 bytes) | Payload (<509 bytes)
Client/ → Onde se encontra o executável do cliente e a subdiretoria onde ficam os ficheiros para envio "/files".
Server/ → Onde se encontra o executável do servidor e a subdiretoria dos ficheiros recebidos "/files".

#### Versões utilizadas
GCC: C17
CMake: 3.10
OS: Ubuntu 20

#### Como executar
```
./server_exec <port> #servidor
./client_exec <server_ip> <server_port> #cliente 
```

## Melhorias
- Mudar as estruturas vector para array
- Criar os packets à medida que envio, não criando um ficheiro e só depois enviar para o servidor.
- Incluir flag no datagrama de último fragmento (evitando o  datagrama vazio para sinalizar)
- Diminuir número de packet types, reutilizando-os.

## Problemas
Devido a um atraso no tratamento dos fragmentos por parte do servidor, o cliente está forçado a esperar 1 milissegundo entre fragmentos. Provocando um atraso significativo ao receber o ficheiro no servidor.
Devido a isso, os ficheiros demoram mais que 1 segundo a chegar ao servidor.

