# Projektbeschreibung

Unter NixOS kann man nicht einfach eine Binary in `/usr/local/bin` ablegen, um sie global verfügbar zu machen. Deshalb habe ich dieses Tool geschrieben. 

Alternativ besteht immer die Möglichkeit, die Binary aus dem Quellcode zu kompilieren, etwa mit einem Makefile, über Rust/Cargo oder aus einzelnen kleinen C-Dateien, die ein eigenständiges Programm bilden.

Das Automatisierungs-Tool für NixOS, führt folgende Schritte aus:

- Kompilierung: Lokale Systemprogramme werden automatisiert gebaut. 
- Paketierung: Das Tool schreibt die passenden Build-Anweisungen für den Paketmanager.
- Deployment: Es wird ein neuer Systemzustand erzeugt.
- Nach dem build in den Systemzustand werden die temporären Kompilate gelöscht.

Root wird nicht zum Kompilieren verwendet, dies findet in einen fork Prozess als Dev-User statt.

# Includes

- main.c <- compile_projects.h <- config_paths.h <- filesystem_operations.h
- main.c <- recreate_dir.h
- main.c <- deploy_pkgs.h <- config_paths.h <- filesystem_operations.h

# Build Makefile

```
make
make clean
make test
```

# Tests

Es gibt umfassende Tests für die Generierung der Config-Dateien und Pfade. Damit wird sichergestellt, dass Konfigurationen und Verzeichnisstrukturen korrekt und reproduzierbar erzeugt werden. Das Programm wurde zusätzlich unter realen Bedingungen getestet, um das Verhalten in praxisnahen Deployment-Szenarien zu verifizieren. Manuelles Ownership wurde eingehalten.
