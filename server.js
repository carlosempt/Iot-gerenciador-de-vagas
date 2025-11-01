// 1. Importa as bibliotecas necessárias
const express = require('express');
const cors = require('cors');

// 2. Inicializa o aplicativo express
const app = express();
const port = process.env.PORT || 3000; // A porta que o servidor vai usar

// 3. Configura os Middlewares
app.use(cors()); // Permite que diferentes origens acessem o servidor
app.use(express.json()); // Permite que o servidor entenda o formato JSON vindo do hardware

// 4. Armazenamento do Estado das Vagas (a "memória" do servidor)
// O servidor começa sabendo que todas as vagas estão disponíveis.
let statusVagas = {
  "1": { "status": "disponivel" },
  "2": { "status": "disponivel" },
  "3": { "status": "disponivel" }
};

// ------------------- ROTAS DA API -------------------

// ROTA 1: Para o WEBSITE pedir o status de todas as vagas
// O site vai chamar: GET http://SEU_IP:3000/status_vagas
app.get('/status_vagas', (req, res) => {
  console.log("Website pediu o status das vagas. Enviando...");
  res.json(statusVagas); // Envia o objeto statusVagas como resposta
});

// ROTA 2: Para o HARDWARE (ESP32) atualizar o status de UMA vaga
// O hardware vai chamar: POST http://SEU_IP:3000/atualizar_vaga
app.post('/atualizar_vaga', (req, res) => {
  // Pega os dados enviados pelo hardware no corpo da requisição
  const { vagaId, status } = req.body;

  // Validação simples para ver se os dados vieram corretamente
  if (!vagaId || !status) {
    return res.status(400).send({ mensagem: "Dados incompletos. Faltou vagaId ou status." });
  }

  // Validação para ver se a vaga existe no nosso sistema
  if (statusVagas[vagaId]) {
    // Atualiza o status da vaga na "memória" do servidor
    statusVagas[vagaId].status = status;
    console.log(`Hardware atualizou a Vaga ${vagaId} para: ${status}`);
    // Envia uma resposta de sucesso para o hardware
    res.status(200).send({ mensagem: `Vaga ${vagaId} atualizada com sucesso para ${status}` });
  } else {
    // Se a vaga não existir, envia um erro
    res.status(404).send({ mensagem: `Vaga ${vagaId} não encontrada.` });
  }
});

// ----------------------------------------------------

// 5. Inicia o servidor para ele começar a "ouvir"
app.listen(port, () => {
  console.log(`Servidor do estacionamento rodando em http://localhost:${port}`);
  console.log("Aguardando conexões do hardware e do site...");
});