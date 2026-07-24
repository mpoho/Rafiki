# Rafiki
Rafiki : Robot compagnon pour enfants (Projet Makers 2026). Propulsé par un Raspberry Pi, il intègre la vision par ordinateur (suivi du regard), un LLM avec gestion de contexte, une synthèse vocale naturelle et un planificateur de routines.

## Developpement local

Installer les dependances dans l'environnement virtuel :

```bash
.venv/bin/python -m pip install -r requirements.txt
```

Lancer les tests unitaires :

```bash
.venv/bin/python -m pytest
```

Verifier l'etat du serveur LLM local sur la Raspberry :

```bash
.venv/bin/python scripts/check_llm.py
```

Faire un essai de generation quand `llama-server` est demarre :

```bash
.venv/bin/python scripts/generate_llm_sample.py
```
