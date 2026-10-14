# Projektbeschreibung

Ein Automatisierungs-Tool für NixOS, das folgende Schritte ausführt:

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

Es gibt umfassende Tests für die Generierung der Config-Dateien und Pfade. Damit wird sichergestellt, dass Konfigurationen und Verzeichnisstrukturen korrekt und reproduzierbar erzeugt werden. Das Programm wurde zusätzlich unter realen Bedingungen getestet, um das Verhalten in praxisnahen Deployment-Szenarien zu verifizieren.

# Speicherverwaltung valgrind Test

Programm läuft von der Speicherverwaltung sauber und sicher. Manuelles Ownership wurde eingehalten.

