# Créditos / Credits

## assets/sounds/brake_loop.wav e assets/sounds/brake_loop_low.wav

Áudios "carreta temperatura alta" (`brake_loop.wav`) e "carreta temperatura baixa"
(`brake_loop_low.wav`) do mod de terceiro **ETS Brake Fade 0.8.1.2**, incluídos
sem alterações. **Não são cobertos pela licença** deste projeto (PolyForm Noncommercial): os direitos pertencem ao
autor original do mod. Foram incluídos por decisão do mantenedor deste repositório, que assume
a responsabilidade por essa redistribuição. Se você é o autor e quer a remoção ou uma
atribuição diferente, abra uma issue e os arquivos serão tratados de imediato.

Audio "carreta temperatura alta" (`brake_loop.wav`) and "carreta temperatura baixa" (`brake_loop_low.wav`) from the
third-party mod **ETS Brake Fade 0.8.1.2**, included unmodified. They are **not covered by this project's license**
(PolyForm Noncommercial): the rights belong to the mod's original author. They were included at the repository
maintainer's decision, who takes responsibility for this redistribution. If you are the author and want them removed
or credited differently, open an issue and the files will be dealt with promptly.

## assets/sounds/air_release.wav

Derivada de **"Truck Air Brakes"**, de **Mike Koenig**,
<https://soundbible.com/1056-Truck-Air-Brakes.html>, licenciada sob
[Creative Commons Atribuição 3.0](https://creativecommons.org/licenses/by/3.0/deed.pt_BR).

**Alterações feitas:** um trecho curto foi recortado, filtrado e equalizado (passa-altas,
filtros notch, passa-baixas) e combinado com uma camada sintetizada de ronco da válvula
(série harmônica modelada a partir de medições espectrais de uma gravação de referência;
nenhum áudio foi copiado dela).

Derived from **"Truck Air Brakes"** by **Mike Koenig**,
<https://soundbible.com/1056-Truck-Air-Brakes.html>, licensed under
[Creative Commons Attribution 3.0](https://creativecommons.org/licenses/by/3.0/).

**Changes made:** a short segment was cut out, filtered and equalized (high-pass, notch
filters, low-pass), and combined with a synthesized valve rumble layer (a harmonic series
modeled from spectral measurements of a reference recording; no audio was copied from it).

## Software de terceiros / Third-party software

O plugin se comunica com o jogo pela interface pública do SDK de telemetria da SCS Software.
`src/scs_sdk_min.h` redeclara apenas as partes necessárias.

The plugin talks to the game through the public SCS Software telemetry SDK interface.
`src/scs_sdk_min.h` re-declares only the parts it needs.

Este projeto não é afiliado nem endossado pela SCS Software. "Euro Truck Simulator" e
"American Truck Simulator" são marcas da SCS Software.

This project is not affiliated with or endorsed by SCS Software. "Euro Truck Simulator"
and "American Truck Simulator" are trademarks of SCS Software.
