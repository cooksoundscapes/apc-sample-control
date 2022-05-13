

* ATÉ AGORA:
    linha 1 reservada p/ trocar de instrumento (ou outra coisa)
    linha 2 = Track List (select track)
    linha 3 a 8 = toggle loop, play slice
    botoes Volume, Send e Device ativam Sliders

* ToDo (C++):
    - Quando Device ON, seleciona Ctrl Page - Como?
        Ativa setas laterais?
        depende de TELA?

    - Select ativa seleção de Sample - precisa de TELA?

    - Regra:
        se Rec Arm = pressed && press numa linha associada c/ track:
            Rec Arm por X compassos, dependendo do botão pressionado:
                1 botao = a 1 tempo? 2 tempos? 1 compasso?
            Rec inicia no início do próximo compasso (ou do outro, caso estiver encima)
            Precisa do Metronomo OK

    - Metronomo PODE ocupar 4 botoes, uma luz por beat
        poderia ocupar até uma linha toda, considerando compasso composto
        Pode também nao ocupar botao e depender da TELA?

    - tap tempo + ajuste fino + -
        poderia ser nos pads, testar responsividade


* ToDo (PD):
    checar ação de speed e trims em sample player

==============================================

1. microlooping
    - gravar por X compassos
    - inicia e acaba no beat
    - NÃO tocar imediatamente
    - chop sequencer
    - pesquisar granular + phase vocoder
    - ADSR p/ slices

2. tap tempo reinicia a contagem!!!

3. nenhum sequencer se reinicia sozinho:
    * o mesmo vale pra loops?

4. metronomo retornando ao zero reinicia sequencers que já acabaram sua sequencia e estão ativos

5. metronomo está SEMPRE ligado

6. toggles momentaneos para FX mais "escandalosos"
    - cummulative pitch shifted delay
    - reverbão
    - bit crushing delay
    - reverse sampling
    - etc etc

7. Para evitar mexer nos TRIMS o máximo possível:
    - patterns podem modificar tempo
    - loops nao podem, só se grava num time frame (evita detune e trimming)

