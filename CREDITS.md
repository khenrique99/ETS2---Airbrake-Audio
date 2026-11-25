# Créditos / Credits

## assets/sounds/brake_loop.wav

Áudio "carreta temperatura alta" do mod de terceiro **ETS Brake Fade 0.8.1.2**, incluído
sem alterações. **Não é coberto pela licença** deste projeto (PolyForm Noncommercial): os direitos pertencem ao
autor original do mod. Foi incluído por decisão do mantenedor deste repositório, que assume
a responsabilidade por essa redistribuição. Se você é o autor e quer a remoção ou uma
atribuição diferente, abra uma issue e o arquivo será tratado de imediato.

Audio "carreta temperatura alta" from the third-party mod **ETS Brake Fade 0.8.1.2**,
included unmodified. It is **not covered by this project's license** (PolyForm Noncommercial): the rights belong
to the mod's original author. It was included at the repository maintainer's decision, who
takes responsibility for this redistribution. If you are the author and want it removed or
credited differently, open an issue and the file will be dealt with promptly.

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
