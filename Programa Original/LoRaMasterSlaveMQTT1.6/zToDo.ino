/*
  Master:
  Decodificar Resposta do Slave ok
  Fazer a estrutura de decodificacao dos DOs/DIs em binarios. (Entra com DigIn ou DigOut do slave de intesse e retorna o valor do sinal). (Implementar isso na biblioteca??) ok
  Fazer Mostrar status da cada I/O no Dash. ok
  Atualizar Status atual dos sinais. ok
(Input APP) Implementar funcao que atualiza DO assim que o usuario mandar solicitacao pelo APP, e enviar solicitacao ao Slave (Callback) ok
(Input APP) Resetar os bool dos outputs no receive e seta-los quando receber ordem do usuario atraves do app. ok
  Modificar funcao de envio de mudanças no APP. Enviar todos os output juntos para tentar corrigir a criptografia. ok

  IF SOMETHING AFTER ":" IS NULL, CODE CRASHES - WebSocket
  Implementar Comandos WebSocket Master
  Tranformar váriaveis de interesse em persistente.
  Fazer pagina de set-up do wifi
  Implementar a estrutura de espera de uma resposta do Slave.
  Gerenciar tempo minimo de envio/resposta para cada Slave (=10s)

  Slave:
  Atualizar sinais da variavel Slave. Hoje atualiza so DO quando recebe DO do master. (IMPORTANTE) ok
  bmodSent - Implementado errado - Nao funcionara quando tiver mais de 1 device. (Jogar ele na classe Slave?) ok
  mensagem enviada pelo slave nao consegue ser decriptografada ok

  Implementar Comandos WebSocket Slave
  Tranformar váriaveis de interesse em persistente.
  (Input APP)  Fazer funcao para atualizar status do pinout. (DO implementada)(Falta AO)
  Quando reiniciado. Puxar status anterior da memoria?
 */

  //REVIEW ALL STRINGS - USE CONCAT INSTEAD OF  +
  //pass the String as references to  functions instead of copies 
  //https://hackingmajenkoblog.wordpress.com/2016/02/04/the-evils-of-arduino-strings/
   
