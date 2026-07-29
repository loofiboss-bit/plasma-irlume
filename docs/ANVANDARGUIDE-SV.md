# Användarguide

KFaceAuth är ett experiment för lokal jämförelse i den redan inloggade
användarsessionen. Ett resultat som säger **Matchning** låser inte upp,
autentiserar inte, anropar inte PAM eller Polkit och ändrar inte Linux-sessionen.

## Ansiktsprofil

Ansiktsinbäddningar är känsliga biometriska uppgifter. Ingen fångad bild sparas
avsiktligt. Profilen krypteras med AES-256-GCM och en slumpmässig nyckel som
endast lagras i KDE KWallet för den inloggade användaren.

Starta förhandsvisningen och registreringen uttryckligen. Varje prov kräver ett
eget klick på **Fånga prov**. Tre prov krävs, fem rekommenderas och åtta är den
hårda gränsen. Inget sparas förrän **Slutför och spara** gör en atomisk
transaktion. Avbrytande, tidsgränsen 120 sekunder, dold sida, stoppad
förhandsvisning eller stängd systeminställning rensar osparade prov.

En giltig profil kan tas bort uttryckligen. Oläsbara data kan återställas efter
en destruktiv bekräftelse; då krävs ny registrering. Borttagning garanterar inte
fysisk radering från SSD, säkerhetskopior, ögonblicksbilder eller
copy-on-write-filsystem.

## Testa igenkänning

Starta förhandsvisningen och välj **Testa en aktuell bildruta**. Endast en
bildruta behandlas. Resultatet kan vara Matchning, Ingen matchning, Tvetydigt
eller ett typat otillgängligt tillstånd. Likhetspoäng visas aldrig.

Det finns ingen livskontroll eller motståndskraft mot presentation/spoofing.
KWallet-nyckeln är inte tillgänglig före inloggning. FAR, FRR, bias,
RGB/IR-säkerhet och autentiseringslämplighet är inte kvalificerade.
Lösenordsreserv är irrelevant eftersom KFaceAuth ännu inte ändrar någon
autentisering.
