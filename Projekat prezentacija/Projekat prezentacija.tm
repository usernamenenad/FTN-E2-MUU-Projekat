<TeXmacs|2.1.1>

<style|beamer>

<\body>
  <\hide-preamble>
    \;

    <assign|by-text|<macro|<inactive|<localize|Student:>>>>

    <assign|enumerate-numeric|<\macro|body>
      <compound|<if|<and|<value|<merge|prefix-|enumerate-numeric>>|<unequal|<value|last-item-nr>|1>>|list*|list>|<macro|name|<aligned-item|<arg|name>.<item-spc>>>|<macro|x|<arg|x>>|<arg|body>>
    </macro>>
  </hide-preamble>

  <screens|<\hidden>
    \;

    \;

    \;

    \;

    \;

    \;

    <doc-data|<doc-title|Mikroprocesorski upravlja£ki
    urežaji>|<\doc-subtitle>
      Projekat \PParking\Q
    </doc-subtitle>|<doc-author|<author-data|<author-name|Nenad Radovi¢,
    RA18/2020>>>>

    \;
  </hidden>|<\hidden>
    <tit|Opis rada rampe i problemi pri projektovanju>

    <section|Opis rada sistema>

    Sistem se sastoji od nekoliko komponenata:

    <\enumerate-numeric>
      <item><em|<strong|<em|Potpatosni prekida£i - >>>govore o prisustvu
      automobila koji, u zavisnosti koji je od pomenutih aktiviran, ºeli u¢i
      na parking ili iza¢i iz njega.

      <item><strong|Grani£ni prekida£i - >govore da li je rampa dostigla vrh
      (odnosno da li je rampa podignuta) ili je rampa dostigla dno (odnosno
      da li je rampa spu²tena).

      <item><strong|Motor - >u zavisnosti od toga koji je aktiviran, de²ava
      se podizanje ili spu²tanje rampe.
    </enumerate-numeric>

    Zahtjev za nesmetan prolazak automobila jeste da se rampa, nakon potpunog
    podizanja, ostane 5 sekundi u datom poloºaju prije nego ²to se po£ne
    spu²tati.

    <section|Problemi pri projektovanju mikrokontrolera>

    Problemi koji se javljaju pri projektovanju mikrokontrolera:

    <\enumerate-numeric>
      <item>Ako automobil ostane na potpatosnom prekida£u nakon perioda
      podizanja i 5 sekundi dato za prolazak, potrebno je obezbjediti da se
      rampa ne spu²ta sve dok automobil nije pro²ao.

      <item>Ako se, pri podizanju rampe, pojavio automobil sa druge strane,
      potrebno je obezbjediti da oba automobila bezbjedno prožu ispod rampe.
    </enumerate-numeric>
  </hidden>|<\hidden>
    <tit|Inicijalizacija mikrokontrolera i izvje²taj o stanju>

    Pored osnovne inicijalizacije mikrokontrolera (inicijalizacije displeja i
    tajmera), potrebno je obezbjediti konstantno aºuriranje stanja sistema,
    kao i slanje stanja preko serijske komunikacije na PC. To se, pored
    prekida, postiºe funkcijom <cpp|displayState()>, koja se poziva pri
    svakoj promjeni stanja.<strong|>

    <\wide-tabular>
      <tformat|<table|<row|<\cell>
        <\cpp-code>
          void displayState() {

          \ \ \ \ \ if(!emergency_stop) {

          \ \ \ \ \ \ \ \ \ writeLine("PR");

          \ \ \ \ \ }

          \ \ \ \ \ else {

          \ \ \ \ \ \ \ \ writeLine("PNR");

          \ \ \ \ \ }

          \ \ \ \ \ newLine();

          \ \ \ \ \ writeLine("BZM: ");

          \ \ \ \ \ if(taken_spots \<less\> 0x09) {

          \ \ \ \ \ \ \ \ writeChar('0');

          \ \ \ \ \ }

          \ \ \ \ \ else {

          \ \ \ \ \ \ \ \ writeChar('1');

          \ \ \ \ \ }

          \ \ \ \ \ writeChar(taken_spots%10 + 0x30);
        </cpp-code>
      </cell>|<\cell>
        <\strong>
          Promjenljive <cpp|emergency_stop>

          i <cpp|taken_spots> govore o tome

          da li je do²lo do zastoja u radu

          rampe zbog neke gre²ke i o\ 

          zauzetosti parkinga, respektivno.

          \;
        </strong>
      </cell>>>>
    </wide-tabular>
  </hidden>|<\hidden>
    <tit|Logika podizanja i spu²tanja rampe (1/4)>

    Ako sistem pravilno radi, flag <cpp|emergency_stop> ¢e biti jednak nuli.
    Sada prolazimo kroz slu£ajeve koji se mogu desiti u sistemu:

    <\itemize>
      <item><strong|Jedan od potpatosnih prekida£a je aktiviran - >za
      ispunjenje ovog uslova, potrebno je da je bar jedan od prekida£a
      aktiviran. Ako je a<strong|>ktiviran potpatosni prekida£ na ulazu,
      rampa ¢e inicirati podizanje <strong|>samo u slu£aju kada postoji
      nezauzeto mjesto na parkingu, dok je u obratnom slu£aju (izlazni
      potpatosni prekida£) bezuslovno aktiviranje.

      <\wide-tabular>
        <tformat|<table|<row|<\cell>
          <\cpp-code>
            if((CS_POT_1 \<gtr\> PS_POT_1 \|\| CS_POT_2 \<gtr\> PS_POT_2)
            \|\| (request_in \|\| request_out)) {

            \ \ if((CS_POT_1 \<gtr\> PS_POT_1 \|\| request_in) && taken_spots
            == 0x0F) {

            \ \ \ \ \ // Ispis da ne postoji vise slobodnih mjesta

            \ \ }

            \ \ else {

            \ \ \ \ \ move_ramp = 1;

            \ \ \ \ \ while(move_ramp) {MOTOR_UP = 1};\ 

            \ \ \ \ \ MOTOR_DOWN = 0;

            \ \ \ \ \ wait1s();

            \ \ \ \ \ if(CSS_GR_2) {

            \ \ \ \ \ \ \ \ wait_ramp = 1;

            \ \ \ \ \ }
          </cpp-code>
        </cell>>>>
      </wide-tabular>
    </itemize>
  </hidden>|<\hidden>
    <tit|Logika podizanja i spu²tanja rampe (2/4)>

    <\wide-tabular>
      <tformat|<table|<row|<\cell>
        <\cpp-code>
          while(wait_ramp \|\| CSS_POT_1 \|\| CSS_POT_2) {

          \ \ \ if(CS_POT_1 \<gtr\> PS_POT_1 && CSS_POT_2) {in_out_flag = 1;}

          \ \ \ if(CS_POT_2 \<gtr\> PS_POT_2 && CSS_POT_1) {in_out_flag = 1;}

          }

          else {emergency_stop = 1; displayState(); continue;}

          if(!in_out_flag) {

          \ \ \ if(CS_POT_1 \<gtr\> PS_POT_1 \|\| request_in) {

          \ \ \ \ \ \ if(++taken_spots == 0x0F) {all_filled = 1;}

          \ \ \ }

          \ \ \ if(CS_POT_2 \<gtr\> PS_POT_2 \|\| request_out) {

          \ \ \ \ \ \ if(--taken_spots == 0x0E) {all_filled = 0;}

          \ \ \ }

          }

          displayState();

          move_ramp = 1;

          while(move_ramp) {MOTOR_DOWN = 1;}

          MOTOR_DOWN = 0;

          wait1s();

          if(!CSS_GR_1) {emergency_stop = 1; displayState(); continue;}

          request_in = 0; request_out = 0; in_out_flag = 0; \ \ 
        </cpp-code>
      </cell>|<\cell>
        \;
      </cell>>>>
    </wide-tabular>
  </hidden>|<\shown>
    <tit|Logika podizanja i spu²tanja rampe (3/4)>

    Kod pokazuje slu£aj kada automobil dože na jedan od potpatosnih
    prekida£a, ºele¢i da uže u ili izaže sa parkinga. Ako, pri ulasku, nije
    mogu¢e u¢i na parking zbog nestanka praznih mjesta, rampa se ne¢e
    podignuti. Trajanje dizanja rampe ponderisano je na 3s. Dok se rampa
    podiºe, aktivirana je dioda (kroz funkciju <cpp|diode_activate()>), dok u
    normalnom stanju dioda svjetli konstantno.

    Poslije dizanja rampe, mikrokontroler se stavlja na £ekanje u trajanju od
    5s, da bi automobili imali vremena da bezbjedno prožu. U datom vremenskom
    intervalu mogu¢ je slu£aj da se pojavi auto sa druge strane od strane
    koja je inicijalno aktivirala rampu, te rampa mora sa£ekati oba auta da
    bezbjedno prožu. Kako se, su²tinski, zauzetost mjesta na parkingu ni
    uve¢ava ni smanjuje, <cpp|<with|prog-font-series|bold|in_out_flag<with|prog-font-series|bold|>>
    >flag-promjenljiva postavlja se na 1, kako se broja£ mjesta ne bi ni
    inkrementirao ni dekrementirao.\ 

    Ako u nekom momentu rada dože do gre²ke u radu rampe, flag-promjenljiva
    <cpp|emergency_stop> postavlja se na 1, te se time nazna£ava da je
    potreban \Prestart\Q u radu rampe. Restart se inicira posebnim tasterom
    vezanom za port 0.

    Potrebno je naglasiti da je mogu¢e pojavljivanje automobila dok restart
    nije iniciran. Zato postoje promjenljive <cpp|request_in> i
    <cpp|request_out>, koje obezbježuju da se \Ppamti\Q stanje potpatosnih
    prekida£a prije restarta.\ 

    Vaºno je napomenuti da, pri restartu rada rampe kontrolera, ²aljemo i na
    UART i na displej kontrolera da dolazi do restarta rampe.
  </shown>|<\hidden>
    <tit|Logika podizanja i spu²tanja rampe (4/4)>

    <\itemize-dot>
      <item><strong|Nijedan od potpatosnih prekida£a nije aktiviran >- u ovom
      slu£aju, mikrokontroler £eka na promjenu stanja. Na displeju, kao i na
      UART-u, sadrºana su trenutna stanja.\ 
    </itemize-dot>
  </hidden>>
</body>

<\initial>
  <\collection>
    <associate|enumerate-levels|1>
    <associate|item-1|<macro|\<bullet\>>>
    <associate|item-nr|2>
    <associate|last-item-nr|0>
    <associate|page-height|auto>
    <associate|page-medium|paper>
    <associate|page-type|4:3>
    <associate|page-width|auto>
  </collection>
</initial>

<\references>
  <\collection>
    <associate|auto-1|<tuple|1|1>>
    <associate|auto-2|<tuple|2|1>>
  </collection>
</references>

<\auxiliary>
  <\collection>
    <\associate|toc>
      <vspace*|1fn><with|font-series|<quote|bold>|math-font-series|<quote|bold>|1<space|2spc>Opis
      rada sistema> <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-1><vspace|0.5fn>

      <vspace*|1fn><with|font-series|<quote|bold>|math-font-series|<quote|bold>|2<space|2spc>Problemi
      pri projektovanju mikrokontrolera> <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-2><vspace|0.5fn>
    </associate>
  </collection>
</auxiliary>