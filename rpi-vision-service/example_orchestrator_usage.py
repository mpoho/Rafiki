"""
Exemple d'intégration pour l'équipe / personne qui gère l'Agent Orchestrateur LLM.
"""
from rpi_vision.client.tool_wrapper import RpiVisionClient, get_vision_tool_definition

# 1. Initialiser le client vers le Raspberry Pi
client = RpiVisionClient(base_url="http://localhost:8000")

# 2. Obtenir la définition de Tool pour OpenAI / LangChain / AutoGen
vision_tool_schema = get_vision_tool_definition()
print("--- Tool Schema pour le LLM ---")
print(vision_tool_schema)
print("-------------------------------\n")

# 3. Simulation de l'appel de Tool par le LLM
try:
    print("Captures de photo en cours depuis le Raspberry Pi...")
    payload = client.capture_image_json(quality=85, width=1280, height=720)
    
    print("\n✅ Photo capturée avec succès !")
    print(f"Horodatage  : {payload['metadata']['timestamp']}")
    print(f"Dimensions  : {payload['metadata']['width']}x{payload['metadata']['height']}")
    print(f"Mode Caméra : {payload['metadata']['camera_type']}")
    print(f"Data URI    : {payload['data_uri'][:60]}... [Base64 Tronqué]")
    
    # 4. Exemple d'insertion dans le payload d'un LLM Vision (ex: OpenAI GPT-4o)
    openai_message = {
        "role": "user",
        "content": [
            {"type": "text", "text": "Analyse cette image capturée par le Raspberry Pi : que vois-tu ?"},
            {
                "type": "image_url",
                "image_url": {
                    "url": payload["data_uri"]
                }
            }
        ]
    }
    print("\n✅ Payload prêt à envoyer au LLM Vision !")

except Exception as e:
    print(f"❌ Erreur de connexion au service Raspberry Pi Vision : {e}")
