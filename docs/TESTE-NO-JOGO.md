# Roteiro de teste no jogo (pt-BR)

Marque o resultado de cada item e cole no PR. Jogo: ETS2 1.62.x, Steam, 64 bits.

## Preparação

1. Extraia o zip em `...\Euro Truck Simulator 2\bin\win_x64\plugins\`. Devem ficar lá:
   `airbrake_audio.dll`, `airbrake_audio.ini`, `AirbrakeAudio.exe` e a pasta `airbrake_audio\` com os três `.wav`.
2. Abra o jogo com `-nointro -developer` e aceite o aviso de **"SDK features"**.
3. Abra `Documentos\Euro Truck Simulator 2\game.log.txt` e procure `[AirbrakeAudio] v... pronto`.
   Se aparecer `Sem som: ...`, o motivo vem na mesma linha (arquivo faltando, formato, sem dispositivo).

## Itens

| # | O que fazer | Esperado | OK? |
|---|---|---|---|
| 1 | Andar a ~50 km/h e frear forte | Ranger do freio em loop, volume segue o pedal | |
| 2 | Soltar o freio depois de uns 6 s | Saída de ar inteira, volume cheio | |
| 3 | Frear só por ~0,6 s e soltar | Saída de ar mais baixa e cortada cedo | |
| 4 | Frear parado (< 3 km/h) | Sem ranger | |
| 5 | Pausar com o freio apertado | Silêncio imediato; ao voltar, volta normal | |
| 6 | Apertar **Ctrl + =** no jogo | O painel abre **dentro do jogo**, por cima da imagem, e o jogo escurece atrás | |
| 7 | No painel, arrastar "Volume máximo" frenando | O volume muda na hora | |
| 8 | Mudar o **Ganho** e soltar o controle | Sons param um instante e voltam com o novo ganho | |
| 9 | **Salvar** e reabrir o jogo | Valores mantidos, comentários do `.ini` preservados | |
| 10 | **Esc**, **Fechar** e **Ctrl + =** (uma vez cada) | O painel fecha; o mouse volta a girar a câmera; nenhuma tecla fica "presa" | |
| 11 | Trocar `hotkey` em `[ui]` no `.ini`, reiniciar o jogo | O novo atalho abre o painel | |
| 12 | Abrir o painel com o jogo em **tela cheia**, **janela** e **sem bordas**; trocar a resolução | Aparece nos três; o tamanho acompanha a resolução | |
| 13 | Convive com o Trailer Control (**Ctrl + -**) | Cada atalho abre só o seu painel | |
| 14 | Frear com o pedal até ~50% | Som de **temperatura baixa**; tela mostra "freio · temp. baixa" | |
| 15 | Aprofundar o pedal para além de 65% | Troca para o som de **temperatura alta** sem corte seco; tela mostra "freio · temp. alta" | |
| 16 | Voltar o pedal para ~50% sem soltar | Volta ao som de temperatura baixa, cruzando suave | |
| 17 | Pisar direto no fundo | Começa já no som de temperatura alta | |
| 18 | Na tela, mudar "Som de temperatura alta a partir de" | O ponto da troca muda na hora | |
| 19 | Frear uns 6 s só abaixo de 65% e soltar | **Sem** saída de ar | |
| 20 | Frear acima de 65% por ~1 s e soltar | **Sem** saída de ar | |
| 21 | Frear acima de 65% por mais de 2 s e soltar | Saída de ar toca | |
| 22 | Com o painel aberto, mexer o mouse e apertar teclas do jogo (W, A, S, D, setas do jogo) | O cursor do painel segue o mouse; o caminhão e a câmera não reagem | |
| 23 | Painel aberto: **↑ ↓** escolhem o ajuste, **← →** mudam o valor, **Tab** troca de aba, **Ctrl + S** salva | Igual ao mouse; o ajuste escolhido fica destacado | |
| 24 | Com o painel aberto, frear e ouvir enquanto muda "Volume máximo" e "Som de temperatura alta a partir de" | O som muda ao vivo; o painel mostra pedal, velocidade e o som tocando | |
| 25 | Botão **EN/PT** no painel; fechar e reabrir | O idioma é lembrado (também vale para o `AirbrakeAudio.exe`) | |
| 26 | `mode = external` em `[ui]` no `.ini`; reiniciar e apertar **Ctrl + =** | Abre a janela separada `AirbrakeAudio.exe`, como antes | |

> **Painel dentro do jogo (beta):** foi testado só com um "jogo falso" em DirectX 11 (`fake_game --overlay`), não no
> ETS2. Se o jogo travar ao abrir o painel, ou o mouse/teclado se comportarem de forma estranha, anote o que
> aconteceu, mande o `game.log.txt` (procure "Painel no jogo") e use `[ui] mode = external` até a correção.

## Sem o jogo

`AirbrakeAudio.exe --demo` simula freadas e responde aos comandos como o plugin.
