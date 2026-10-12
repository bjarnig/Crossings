( // walk of waveset lengths, sine
{ WsGen.ar(Dbrown(80, 900, 40), 0.3, 0) ! 2 }.play;
)

( // shape morph under mouse, skew drawn per waveset
{ WsGen.ar(220, 0.3, MouseX.kr(0, 3), Dbrown(0.1, 0.9, 0.05)) ! 2 }.play;
)

( // sequenced lengths, repeats as rhythm
{ WsGen.ar(Dseq([110, 220, 165, 330], inf), Dwhite(0.1, 0.4), Dwhite(0, 3), 0.5, Dseq([8, 4, 16], inf)) ! 2 }.play;
)
