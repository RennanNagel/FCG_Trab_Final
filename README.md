# 🎮 Trabalho final Fundamentos de Computação Gráfica

**Trabalho final da disciplina INF01047 - Fundamentos de Computação Gráfica**  
Professor: Eduardo Gastal  
Semestre: 2025/1 - Universidade Federal do Rio Grande do Sul (UFRGS)  
**Integrantes:** Rennan Nagel e Lucas Nogueira

---

##  Sobre o Jogo

Neste labirinto seu objetivo é resgatar a vaquinha, desvendando um labirinto e fugindo de inimigos

---

## 👩‍💻 Contribuições

Rennan: implementação da fog, curvas de bézier e free camera.

Lucas: criação de shaders e vertex GLSL, lógica do labirinto, carregamento de modelos, implementação de iluminação para os modelos e base do projeto.

---

## 🤖 Uso do ChatGPT

ChatGPT foi utilizado **apenas para sanar dúvidas** pontuais, relacionadas à:
- Debug do código
- Eventuais dúvidas sobre implementações de câmera e iluminação

---

## 🧱 Recursos Implementados

- **Malhas poligonais complexas:** Utilizadas em diferentes objetos do cenário.
- **Transformações geométricas:** Aplicadas para movimentação do jogador sobre o plano.
- **Câmeras:**
  - **Câmera livre:** Para visualização geral do labirinto.
  - **Câmera look-at:** Para acompanhar a orientação do jogador.
- **Instâncias de objetos:** Objetos como *Ghost* e *Maze* são reutilizados com a mesma malha.
- **Mapeamento de texturas:** Todos os fantasmas, paredes e a vaca tem suas texturas mapeadas para eles.
- **Curvas de Bézier:** Bézier cúbica utilizada para animar o movimento da transição entre câmeras.
- **Animações baseadas no tempo:** Todos os movimentos do jogo (câmeras, jogador, vaca, inimigos, transição de câmeras) são animados com base no tempo.

---

## 🎮 Controles

| Tecla / Ação            | Função                           |
|-------------------------|----------------------------------|
| `Q`                     | Fecha o jogo                     |
| `W`                     | Move o jogador para frente       |
| `S`                     | Move o jogador para trás         |
| `D`                     | Move o jogador para a direita    |
| `A`                     | Move o jogador para a esquerda   |
| `C`                     | Alterna entre os modos de câmera |
| `Movimento do mouse`    | Controla a câmera                |

---

## ⚙️ Compilação e Execução

### 🪟 Linux

- Através do terminal use o comando "cmake ." dentro da pasta do projeto
- Após isso utilize o comando "make run" para executar o código

---

## 📸 Capturas de tela

![WhatsApp Image 2025-07-09 at 21 37 32](https://github.com/user-attachments/assets/d78de10f-33f0-4cb8-8bcb-f96291a9eb42)
![WhatsApp Image 2025-07-09 at 21 37 59](https://github.com/user-attachments/assets/3500f14b-ee1d-442b-af97-989ba2f8c279)


---

## 📝 Licença

Este projeto foi desenvolvido exclusivamente para fins acadêmicos e não possui uma licença formal.

---
