# Pac-Man — DE10-Standard (ARM Cortex-A9)

Implementação do jogo clássico **Pac-Man** rodando nativamente no processador **ARM Cortex-A9 (HPS)** da placa **Intel DE10-Standard**, escrita inteiramente em **C**, sem bibliotecas gráficas de alto nível — apenas acesso direto aos registradores mapeados em memória do *frame buffer* de vídeo, botões, *switches* e temporizador da placa.

Projeto desenvolvido para a disciplina de **Projeto de Sistemas Computacionais Embarcados**.

> UFSCar — Sist. Embarcados
> Autores: Lucas Vasconcelos Fujii (RA: 814291) · João Vitor Torcato Arroyo (RA: 814135)
> Professor: Dr. Emerson Carlos Pedrino

---

## Índice

- [Demonstração](#demonstração)
- [Visão geral](#visão-geral)
- [Hardware necessário](#hardware-necessário)
- [Estrutura do repositório](#estrutura-do-repositório)
- [Pré-requisitos de software](#pré-requisitos-de-software)
  - [1. WSL + Ubuntu (Windows 11)](#1-wsl--ubuntu-windows-11)
  - [2. Quartus Prime Lite + University Program](#2-quartus-prime-lite--university-program)
- [Como obter o código](#como-obter-o-código)
- [Como compilar e executar](#como-compilar-e-executar)
  - [Criando o projeto no Monitor Program](#criando-o-projeto-no-monitor-program)
  - [Compilando](#compilando)
  - [Gravando e executando](#gravando-e-executando)
- [⚠️ Problema conhecido: trava em `main: init symbols`](#️-problema-conhecido-trava-em-main-init-symbols)
- [Controles do jogo](#controles-do-jogo)
- [Como o jogo funciona](#como-o-jogo-funciona)
  - [Mapa de memória / periféricos usados](#mapa-de-memória--periféricos-usados)
  - [Camada de vídeo (`video_hal`)](#camada-de-vídeo-video_hal)
  - [Entrada (`input_hal.h`)](#entrada-input_halh)
  - [Temporização (`timer_hal.h`)](#temporização-timer_halh)
  - [Lógica do jogo (`pacman_game.h`)](#lógica-do-jogo-pacman_gameh)
  - [Sprites (`sprites.h`)](#sprites-spritesh)
  - [Loop principal (`main.c`)](#loop-principal-mainc)
- [Personalização](#personalização)
- [Solução de problemas (Troubleshooting)](#solução-de-problemas-troubleshooting)
- [Limitações conhecidas](#limitações-conhecidas)
- [Créditos e referências](#créditos-e-referências)
- [Licença](#licença)

---

## Demonstração

O jogo exibe, via saída VGA (resolução lógica 320×240, upscale automático para 640×480), um labirinto azul clássico de Pac-Man, com o jogador (círculo amarelo com "boca" direcional) sendo perseguido por 4 fantasmas (com o tema visual de "pão de queijo com Nutella" 🧀), pastilhas para coletar, *power pellets* que assustam os fantasmas, placar de pontuação, vidas e fase no topo da tela.

```
SCORE:300      VIDAS:3      FASE:1                    PAC-MAN UFSCar
        === PAC-MAN ===
        UFSCar - Sist. Embarcados

  ██████████████████████
  █o..........  ..o.█
  █.██.███████.██..█.█
  ...
```

---

## Visão geral

| Item | Descrição |
|---|---|
| **Placa alvo** | Intel DE10-Standard (FPGA Cyclone V + HPS ARM Cortex-A9) |
| **Linguagem** | C (padrão C99) |
| **Toolchain** | `arm-altera-eabi-gcc` (fornecida pelo Intel FPGA Monitor Program) |
| **Saída gráfica** | VGA, resolução lógica 320×240 (upscale automático conforme o *frame buffer* detectado) |
| **Entrada** | 4 botões `KEY[3:0]` (movimento) + `SW0` (start) |
| **Frame rate** | 60 Hz (loop principal), IA dos fantasmas a 30 Hz |
| **Sem SO** | Programa *bare-metal*, sem sistema operacional — roda direto sobre o HPS via *preloader* |

---

## Hardware necessário

- Placa **Intel DE10-Standard**;
- Monitor com entrada **VGA** e cabo VGA;
- Cabo **USB** (USB-Blaster / DE-SoC) para programação e depuração via JTAG;
- Computador host (testado em **Windows 11**).

---

## Estrutura do repositório

```
.
├── main.c                 # Loop principal do jogo (única fonte .c junto com video_hal.c)
├── video_hal.c             # Implementação da camada de vídeo (frame buffer)
├── video_hal.h             # Interface da camada de vídeo
├── input_hal.h              # Leitura de KEY[3:0] e SW0
├── timer_hal.h               # ARM A9 Private Timer / controle de FPS
├── sprites.h                  # Desenho do Pac-Man, fantasmas e placar
├── pacman_game.h               # Mapa, estados, física de colisão e IA dos fantasmas
├── address_map_arm.h            # Mapa de endereços de todos os periféricos da placa
├── arroyo.amp                    # Arquivo de projeto do Intel FPGA Monitor Program
├── makefile                       # Makefile de compilação (gerado pelo Monitor Program)
├── amp.mk                          # Makefile estendido (compilação + gravação/depuração)
└── .gitignore
```

> **Importante:** apenas `main.c` e `video_hal.c` são arquivos-fonte (`.c`). Todos os `.h` ficam na mesma pasta e são incluídos via `#include` — **não** devem ser adicionados individualmente como *source files* no projeto do Monitor Program.

---

## Pré-requisitos de software

### 1. WSL + Ubuntu (Windows 11)

O *Intel FPGA Monitor Program* não recebe suporte ativo da Intel desde o final de 2021 e apresenta um problema de compatibilidade conhecido em máquinas Windows 11 (veja a seção [Problema conhecido](#️-problema-conhecido-trava-em-main-init-symbols)). Para contornar isso, é necessário ter o WSL com Ubuntu disponível.

```powershell
# PowerShell como administrador
wsl --install
# ou, se já houver alguma distro instalada:
wsl --install -d Ubuntu
```

Após reiniciar e concluir a configuração inicial do Ubuntu (usuário/senha), garanta que o comando `make` esteja disponível dentro da distro:

```bash
sudo apt update
sudo apt install make
```

### 2. Quartus Prime Lite + University Program

Instale o **Quartus Prime Lite** (versão utilizada: `25.1std`) com o pacote adicional **University Program**, garantindo que os seguintes componentes sejam selecionados durante a instalação:

- Suporte a dispositivo **Cyclone V**;
- **University Program → Computer Systems** (sistema *DE10-Standard Computer for ARM Cortex-A9*, arquivo `.sof`);
- **University Program → Monitor Program** (inclui a *toolchain* `arm-altera-eabi-gcc`);
- Drivers **USB-Blaster / JTAG**.

Conecte a placa via USB e confirme, no Quartus Programmer, que o cabo **`DE-SoC [USB-1]`** é reconhecido.

---

## Como obter o código

```bash
git clone https://github.com/startwotwo/Pacman_DE_10_Standard.git
cd Pacman_DE_10_Standard
```

---

## Como compilar e executar

### Criando o projeto no Monitor Program

1. Abra o **Intel FPGA Monitor Program**.
2. `File → New Project...`
3. Tipo de projeto: **C Program**, arquitetura: **ARM Cortex-A9**.
4. Sistema: **DE10-Standard Computer for ARM Cortex-A9** (o Monitor Program deve localizar automaticamente o `.sopcinfo` e o `.sof` do sistema, instalados junto ao University Program).
5. Aponte o diretório do projeto para a pasta onde o repositório foi clonado.
6. Em **Source files**, adicione **apenas**:
   - `main.c`
   - `video_hal.c`
7. Mantenha as *flags* de compilação padrão (`-g -O1`).
8. Em **System parameters**, confirme:
   - **Host connection:** `DE-SoC [USB-1]`
   - **Processor:** `ARM_A9_HPS_arm_a9_0`
9. Finalize o assistente (**Save**). O Monitor Program perguntará se deseja gravar o sistema (`.sof`) na placa agora — responda **Sim**, caso ainda não tenha gravado o sistema base.

Esse fluxo gera/atualiza o arquivo de projeto `arroyo.amp`, já presente neste repositório.

### Compilando

Use a opção de compilação (**Build** / alvo `COMPILE`) do próprio Monitor Program. Internamente, ela executa o `makefile`/`amp.mk` via `arm-altera-eabi-gcc`, produzindo em sequência:

```
main.c.o  video_hal.c.o   →   main.axf   →   main.srec
```

`main.srec` é o binário final, no formato Motorola S-record, que será carregado na memória DDR3 da placa.

Também é possível compilar manualmente pela linha de comando (dentro do ambiente que possui a *toolchain* `arm-altera-eabi-gcc` no `PATH`):

```bash
make -f makefile COMPILE
```

### Gravando e executando

Pela **interface padrão** do Monitor Program: use o botão de *download & run* (ícone de seta ▶ na barra de ferramentas), que carrega o `main.srec` e inicia a execução no processador ARM.

> Em máquinas Windows 11, esse fluxo pode travar — veja a seção abaixo.

---

## ⚠️ Problema conhecido: trava em `main: init symbols`

Ao usar o Monitor Program em uma máquina **Windows 11** (com WSL instalado), o processo de carregamento trava na etapa **`main: init symbols`**, aparentemente por conflito de comunicação relacionado ao WSL. A causa raiz não é documentada oficialmente (a ferramenta não recebe suporte ativo desde 2021), mas o comportamento é reproduzível.

**Contorno testado e funcional:** usar o **GDB Server** em vez da rotina padrão de *download/run* da GUI.

1. No Monitor Program, troque o modo de execução/depuração para **GDB Server**.
2. Quando a interface travar (ou antes disso, ao iniciar a sessão de GDB), abra o console do GDB Server e execute, em sequência:

   ```gdb
   monitor reset
   load
   continue
   ```

   - `monitor reset` — reinicia o processador ARM;
   - `load` — transfere `main.srec` para a memória DDR3 da placa;
   - `continue` — retoma a execução a partir do ponto de entrada do programa.

3. **A GUI do Monitor Program pode continuar travada/sem resposta após isso — isso é esperado.** O programa é carregado e executado corretamente na placa mesmo assim; a trava afeta apenas o acompanhamento visual na interface, não a execução real no hardware.

Se precisar reiniciar o jogo, repita os três comandos acima (`monitor reset` / `load` / `continue`) no console do GDB Server.

---

## Controles do jogo

| Controle | Ação |
|---|---|
| `KEY0` | Mover para **cima** |
| `KEY1` | Mover para **baixo** |
| `KEY2` | Mover para **esquerda** |
| `KEY3` | Mover para **direita** |
| `SW0` (levantar) | **Iniciar** / **reiniciar** o jogo |

Os `KEY` são ativos em nível baixo (0 = pressionado); `SW0` é ativo em nível alto (1 = ligado), com detecção de borda de subida para evitar reinícios múltiplos enquanto o *switch* permanece levantado.

---

## Como o jogo funciona

### Mapa de memória / periféricos usados

Definidos em `address_map_arm.h` (arquivo padrão da Intel University Program). Os principais endereços usados pelo jogo:

| Periférico | Endereço | Uso no jogo |
|---|---|---|
| `LED_BASE` | `0xFF200000` | Pisca LEDs indicando que o loop principal está rodando |
| `KEY_BASE` | `0xFF200050` | Leitura dos 4 botões (movimento) |
| `SW_BASE` | `0xFF200040` | Leitura do `SW0` (start) |
| `PIXEL_BUF_CTRL_BASE` | `0xFF203020` | Controle do *frame buffer* de pixels (VGA) |
| `CHAR_BUF_CTRL_BASE` / `FPGA_CHAR_BASE` | `0xFF203030` / `0xC9000000` | Buffer de caracteres (placar e textos) |
| `RGB_RESAMPLER_BASE` | `0xFF203010` | Detecção da profundidade de cor do *frame buffer* |
| `MPCORE_PRIV_TIMER` | `0xFFFEC600` | ARM A9 Private Timer (base de tempo / 60 FPS) |

### Camada de vídeo (`video_hal`)

- `video_init()` — detecta resolução e profundidade de cor do *frame buffer* atual;
- `video_box()` / `video_text()` / `resample_rgb()` / `get_data_bits()` — **idênticas** às funções oficiais do `video.c` da Intel University Program;
- `video_pixel()` — desenha um pixel individual, reaproveitando o cálculo de endereço de `video_box()`;
- `video_circle()` — círculo preenchido (testa `x² + y² ≤ r²` para cada pixel do quadrado delimitador);
- `video_clear()` — limpa a tela inteira com uma cor RGB24.

Resolução lógica do jogo: **320×240** pixels (escalada automaticamente para 640×480 na saída VGA real, via `res_offset`/`col_offset`).

### Entrada (`input_hal.h`)

- `input_get_direction()` — lê `KEY[3:0]` (ativo-baixo) e retorna a direção correspondente, com prioridade `KEY0 > KEY1 > KEY2 > KEY3`;
- `input_start()` — retorna 1 se `SW0` estiver levantado;
- `input_start_edge()` — detecta borda de subida de `SW0`, usada para iniciar/reiniciar o jogo sem repetição indevida.

### Temporização (`timer_hal.h`)

Usa o **ARM A9 Private Timer** (`MPCORE_PRIV_TIMER`) em modo contínuo (*auto-reload*), fornecendo:

- `timer_delay_ms()` — espera ocupada (*busy-wait*) em milissegundos, usada em mensagens como `READY!` e `MORREU!`;
- `FrameTimer` / `frame_timer_wait()` — mantém o loop principal travado em **60 quadros por segundo**.

### Lógica do jogo (`pacman_game.h`)

- **Mapa:** matriz de caracteres 20×15 *tiles* (`MAZE_TEMPLATE`), cada *tile* de 16×16 pixels. Símbolos: `#` parede, `.` pastilha (10 pts), `o` *power pellet* (50 pts + assusta fantasmas), espaço = corredor vazio;
- **Colisão com paredes:** `can_move()` verifica os quatro cantos da próxima posição do sprite (margem de 6 px) contra o mapa;
- **Coleta de pastilhas:** `collect_pellet()` — soma pontos e, no caso de *power pellet*, ativa o estado "assustado" nos 4 fantasmas por 300 *frames*;
- **IA dos fantasmas (`ghost_ai()`):**
  - cada fantasma tem uma semente de aleatoriedade própria, atualizada por um gerador congruente linear simples a cada *frame*;
  - na maior parte do tempo mantém a direção atual; em cruzamentos há 1/4 de chance de tentar virar;
  - evita meia-volta, exceto quando não há alternativa;
  - quando assustado, prioriza direções que o afastam do jogador;
- **Colisão jogador-fantasma (`check_ghost_collision()`):** por distância em pixels; se o fantasma estiver assustado, é "comido" (desativado + 200 pts); caso contrário, o jogador perde uma vida;
- **`game_init()`:** (re)inicializa mapa, jogador, fantasmas e contadores para uma nova fase/partida.

### Sprites (`sprites.h`)

- `draw_pacman()` — círculo amarelo com uma "boca" recortada por rotação de coordenadas conforme a direção do movimento;
- `draw_ghost()` — fantasma temático "pão de queijo com Nutella": semicírculo + corpo retangular, camada escura no topo simulando recheio, "saias" onduladas na base, olhos brancos com pupila azul;
- `draw_ghost_scared()` — alterna azul/branco (efeito de piscar) quando o fantasma está assustado;
- `draw_score()` — escreve pontuação, vidas, fase e título diretamente no *character buffer* (`FPGA_CHAR_BASE`).

### Loop principal (`main.c`)

1. Inicializa LEDs, vídeo e temporizador (60 Hz);
2. Exibe a tela de atração (`tela_attract()`) até detectar borda de subida em `SW0`;
3. Inicializa o jogo (`game_init()`) e entra no loop principal, que a cada *frame*:
   - lê o input e atualiza a posição do jogador (`update_player()`);
   - atualiza a IA dos fantasmas a 30 Hz;
   - verifica colisões (perda de vida / fantasma comido);
   - trata *game over* e reinício;
   - verifica vitória de fase (todas as pastilhas coletadas) e avança de nível;
   - renderiza o *frame* (`render_frame()`) usando a técnica de **dirty rectangle** — apaga e redesenha apenas as áreas ocupadas pelas posições anteriores dos sprites, em vez da tela inteira, mantendo desempenho fluido em uma CPU sem GPU dedicada.

---

## Personalização

| O que mudar | Onde |
|---|---|
| Layout do labirinto | `MAZE_TEMPLATE` em `pacman_game.h` (mantenha 20 colunas × 15 linhas e borda de `#`) |
| Velocidade do jogador/fantasmas | variável `speed` em `update_player()` (`main.c`) e `ghost_ai()` (`pacman_game.h`) |
| Cores dos fantasmas | `GHOST_COLOR_0..3` em `pacman_game.h` |
| Número de vidas iniciais | `g->player.lives` em `game_init()` (`pacman_game.h`) |
| FPS do loop principal | argumento de `frame_timer_init(&ft, 60)` em `main()` |
| Duração do *power pellet* | `power_timer` / `scared_timer` (valor `300`, em `pacman_game.h`) |
| Textos na tela | `tela_attract()` e mensagens (`READY!`, `MORREU!`, etc.) em `main.c` |

---

## Solução de problemas (Troubleshooting)

| Sintoma | Causa provável | Solução |
|---|---|---|
| Monitor Program trava em `main: init symbols` | Incompatibilidade conhecida com Windows 11/WSL | Usar GDB Server + `monitor reset` / `load` / `continue` (ver seção acima) |
| Cabo `DE-SoC [USB-1]` não aparece | Driver USB-Blaster não instalado ou placa desligada/desconectada | Reinstalar drivers via Quartus Programmer; verificar alimentação e cabo USB |
| Tela preta / nada aparece no monitor | Sistema base (`.sof`) não gravado na placa antes do programa em C | Regravar o sistema `DE10-Standard_Computer.sof` (opção "Download System" no assistente do projeto) |
| Erros de compilação sobre arquivos `.h` não encontrados | `.h` adicionados como *source files* por engano | Remover os `.h` da lista de *source files* — apenas `main.c` e `video_hal.c` devem estar lá |
| Jogo não inicia mesmo levantando `SW0` | `SW0` já estava levantado antes do boot (o programa espera uma transição 0→1) | Abaixe e levante `SW0` novamente após o carregamento |
| Pac-Man atravessa paredes ou fica preso em corredores | Margem de colisão desalinhada com o tamanho do *tile* após alteração no mapa | Ajustar a margem `m` em `can_move()` (`pacman_game.h`) |

---

## Limitações conhecidas

- Não há efeitos sonoros (a placa DE10-Standard possui codec de áudio, mas não foi utilizado neste projeto);
- A IA dos fantasmas é intencionalmente simples (pseudoaleatória), sem os padrões de perseguição clássicos (Blinky/Pinky/Inky/Clyde do arcade original);
- O jogo é *bare-metal*: não há sistema operacional, *watchdog* ou tratamento de falhas de hardware — um travamento no HPS exige `monitor reset` + `load` novamente.

---

## Créditos e referências

- Intel Corporation — *DE10-Standard User Manual* (Intel FPGA University Program);
- Intel Corporation — *Intel FPGA Monitor Program* / *video.c* de referência (University Program, Quartus Prime Lite);
- Microsoft — [Documentação do WSL](https://learn.microsoft.com/windows/wsl/).

---

## Licença

Este projeto foi desenvolvido para fins acadêmicos na disciplina de Projeto de Sistemas Computacionais Embarcados. Sinta-se à vontade para estudar, adaptar e reutilizar o código, citando os autores originais.
