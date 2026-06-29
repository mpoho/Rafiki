# Télécharger les modèles Vosk

Créer le dossier local des modèles :

```powershell
mkdir models\vosk
```

Télécharger puis décompresser :

- Français : https://alphacephei.com/vosk/models/vosk-model-small-fr-0.22.zip
- Anglais : https://alphacephei.com/vosk/models/vosk-model-small-en-us-0.15.zip

Structure attendue depuis le dossier du module Python :

```text
models/
  vosk/
    vosk-model-small-fr-0.22/
    vosk-model-small-en-us-0.15/
```

Ces fichiers sont lourds et ne doivent pas être commités.