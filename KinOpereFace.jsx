'use client';

import React, { useState, useEffect, useRef } from 'react';
import './KinOpereFace.css';

// Base de données des presets d'émotions adaptée au fond blanc
const EMOTIONS = {
  neutral: {
    openness: 1.0, pupil_size: 0.38, brow_tilt: 0.0, eyelid_curve: 0.05, glow: 0.45, blink_interval: 3.8,
    accent: "#0ea5e9", background: "#ffffff", spark: "#e0f2fe",
    width: 138, height: 168, openScale: 1, widthScale: 1, slant: 4, leftTop: 16, rightTop: 84, leftBottom: 14, rightBottom: 86,
    topInset: 0, bottomInset: 0, sideTop: 18, sideBottom: 82, radiusTopOuter: 24, radiusTopInner: 24, radiusBottomOuter: 22, radiusBottomInner: 22
  },
  happy: {
    openness: 0.82, pupil_size: 0.34, brow_tilt: -0.18, eyelid_curve: 0.34, glow: 0.7, blink_interval: 2.8,
    accent: "#22c55e", background: "#ffffff", spark: "#dcfce7",
    width: 144, height: 156, openScale: 0.92, widthScale: 1.08, slant: 12, leftTop: 12, rightTop: 88, leftBottom: 10, rightBottom: 90,
    topInset: 10, bottomInset: 0, sideTop: 24, sideBottom: 82, radiusTopOuter: 34, radiusTopInner: 34, radiusBottomOuter: 18, radiusBottomInner: 18
  },
  sad: {
    openness: 0.62, pupil_size: 0.36, brow_tilt: 0.2, eyelid_curve: -0.22, glow: 0.3, blink_interval: 5.2,
    accent: "#3b82f6", background: "#ffffff", spark: "#dbeafe",
    width: 140, height: 150, openScale: 0.88, widthScale: 1, slant: -10, leftTop: 14, rightTop: 86, leftBottom: 8, rightBottom: 92,
    topInset: 6, bottomInset: 6, sideTop: 18, sideBottom: 88, radiusTopOuter: 18, radiusTopInner: 30, radiusBottomOuter: 24, radiusBottomInner: 20
  },
  angry: {
    openness: 0.72, pupil_size: 0.3, brow_tilt: 0.42, eyelid_curve: -0.1, glow: 0.78, blink_interval: 2.3,
    accent: "#ef4444", background: "#ffffff", spark: "#fee2e2",
    width: 150, height: 128, openScale: 0.8, widthScale: 1.12, slant: 20, leftTop: 8, rightTop: 92, leftBottom: 20, rightBottom: 80,
    topInset: 8, bottomInset: 8, sideTop: 18, sideBottom: 82, radiusTopOuter: 12, radiusTopInner: 12, radiusBottomOuter: 22, radiusBottomInner: 22
  },
  surprised: {
    openness: 1.0, pupil_size: 0.26, brow_tilt: -0.28, eyelid_curve: 0.02, glow: 0.88, blink_interval: 4.6,
    accent: "#eab308", background: "#ffffff", spark: "#fef9c3",
    width: 130, height: 178, openScale: 1.08, widthScale: 0.96, slant: 0, leftTop: 18, rightTop: 82, leftBottom: 18, rightBottom: 82,
    topInset: 0, bottomInset: 0, sideTop: 14, sideBottom: 86, radiusTopOuter: 30, radiusTopInner: 30, radiusBottomOuter: 30, radiusBottomInner: 30
  },
  sleepy: {
    openness: 0.34, pupil_size: 0.42, brow_tilt: 0.08, eyelid_curve: -0.28, glow: 0.24, blink_interval: 1.8,
    accent: "#a855f7", background: "#ffffff", spark: "#f3e8ff",
    width: 164, height: 76, openScale: 0.58, widthScale: 1.14, slant: 0, leftTop: 14, rightTop: 86, leftBottom: 14, rightBottom: 86,
    topInset: 22, bottomInset: 22, sideTop: 34, sideBottom: 66, radiusTopOuter: 24, radiusTopInner: 24, radiusBottomOuter: 24, radiusBottomInner: 24
  },
  curious: {
    openness: 0.94, pupil_size: 0.33, brow_tilt: -0.08, eyelid_curve: 0.12, glow: 0.6, blink_interval: 3.1,
    accent: "#06b6d4", background: "#ffffff", spark: "#ecfeff",
    width: 148, height: 152, openScale: 0.96, widthScale: 1.04, slant: 8, leftTop: 18, rightTop: 82, leftBottom: 12, rightBottom: 88,
    topInset: 4, bottomInset: 0, sideTop: 20, sideBottom: 84, radiusTopOuter: 22, radiusTopInner: 28, radiusBottomOuter: 18, radiusBottomInner: 18
  },
  love: {
    openness: 0.86, pupil_size: 0.28, brow_tilt: -0.22, eyelid_curve: 0.28, glow: 0.82, blink_interval: 2.6,
    accent: "#ec4899", background: "#ffffff", spark: "#fce7f3",
    width: 144, height: 148, openScale: 0.9, widthScale: 1.06, slant: 6, leftTop: 16, rightTop: 84, leftBottom: 10, rightBottom: 90,
    topInset: 6, bottomInset: 6, sideTop: 22, sideBottom: 84, radiusTopOuter: 28, radiusTopInner: 28, radiusBottomOuter: 26, radiusBottomInner: 26
  },
  thinking: {
    openness: 0.78, pupil_size: 0.32, brow_tilt: 0.12, eyelid_curve: 0.0, glow: 0.52, blink_interval: 4.3,
    accent: "#10b981", background: "#ffffff", spark: "#d1fae5",
    width: 152, height: 118, openScale: 0.76, widthScale: 1.18, slant: 10, leftTop: 14, rightTop: 86, leftBottom: 8, rightBottom: 92,
    topInset: 18, bottomInset: 12, sideTop: 28, sideBottom: 74, radiusTopOuter: 18, radiusTopInner: 18, radiusBottomOuter: 18, radiusBottomInner: 18
  }
};

// Sourcils expressifs
const EYEBROW_PRESETS = {
  neutral: { leftY: 0, rightY: 0, leftRot: 0, rightRot: 0 },
  happy: { leftY: -10, rightY: -10, leftRot: -8, rightRot: 8 },
  sad: { leftY: 5, rightY: 5, leftRot: -15, rightRot: 15 },
  angry: { leftY: 8, rightY: 8, leftRot: 18, rightRot: -18 },
  surprised: { leftY: -18, rightY: -18, leftRot: -3, rightRot: 3 },
  sleepy: { leftY: 6, rightY: 6, leftRot: 0, rightRot: 0 },
  curious: { leftY: -10, rightY: 2, leftRot: -10, rightRot: 6 },
  love: { leftY: -9, rightY: -9, leftRot: -6, rightRot: 6 },
  thinking: { leftY: -5, rightY: 6, leftRot: -5, rightRot: -12 }
};

const clamp = (val, min, max) => Math.max(min, Math.min(val, max));

export default function KinOpereFace({
  controlTopic = 'kinopere/robot/control',
  displayTopic = 'kinopere/robot/display',
  brokerUrl = 'broker.hivemq.com',
  brokerPortWs = 8000,
  brokerPortWss = 8010
}) {
  const [emotion, setEmotion] = useState('neutral');
  const [pupilX, setPupilX] = useState(0.0);
  const [pupilY, setPupilY] = useState(0.0);
  
  const [isBlinking, setIsBlinking] = useState(false);
  const [isWalking, setIsWalking] = useState(false);
  const [isDancing, setIsDancing] = useState(false);
  const [isCliffDetected, setIsCliffDetected] = useState(false);

  // Nouvel état Multi-vues et Contenus
  const [currentView, setCurrentView] = useState('eyes'); // 'eyes' | 'text' | 'image'
  const [displayText, setDisplayText] = useState('HI');
  const [displayImageUrl, setDisplayImageUrl] = useState('');
  const [inputTextVal, setInputTextVal] = useState('6x6=32');
  const [inputImageUrlVal, setInputImageUrlVal] = useState('education_graphic.jpg');

  const [mqttConnected, setMqttConnected] = useState(false);
  const [lastMessage, setLastMessage] = useState('Aucun');
  const [errorMsg, setErrorMsg] = useState(null);

  const mqttClientRef = useRef(null);

  // 1. Connexion MQTT (SSR Safe)
  useEffect(() => {
    if (typeof window === 'undefined') return;

    const connectMqtt = async () => {
      try {
        const mqtt = (await import('mqtt')).default;
        const isHttps = window.location.protocol === 'https:';
        const protocol = isHttps ? 'wss' : 'ws';
        const port = isHttps ? brokerPortWss : brokerPortWs;
        const targetUrl = `${protocol}://${brokerUrl}:${port}/mqtt`;

        console.log(`[MQTT] Connexion en cours vers : ${targetUrl}`);
        
        const client = mqtt.connect(targetUrl, {
          clientId: `kinopere_react_${Math.random().toString(16).substring(2, 8)}`,
          clean: true,
          connectTimeout: 4000,
          reconnectPeriod: 2000
        });

        mqttClientRef.current = client;

        client.on('connect', () => {
          setMqttConnected(true);
          setErrorMsg(null);
          client.subscribe([controlTopic, displayTopic, statusTopic]);
        });

        client.on('message', (topic, message) => {
          const payload = message.toString().trim();
          setLastMessage(`[${topic}] "${payload}"`);
          
          if (topic === displayTopic) {
            handleDisplayPayload(payload);
          } else {
            handleMqttPayload(payload);
          }
        });

        client.on('close', () => setMqttConnected(false));
        client.on('error', (err) => {
          setErrorMsg(err.message);
          setMqttConnected(false);
        });
      } catch (err) {
        setErrorMsg('Échec chargement MQTT.');
      }
    };

    connectMqtt();

    return () => {
      if (mqttClientRef.current) {
        mqttClientRef.current.end();
      }
    };
  }, [controlTopic, displayTopic, brokerUrl, brokerPortWs, brokerPortWss]);

  // 2. minuteur de clignement automatique
  useEffect(() => {
    // Désactive les clignotements si sommeil ou si écran non-yeux
    if (emotion === 'sleepy' || currentView !== 'eyes') {
      setIsBlinking(false);
      return;
    }

    let blinkTimeout;
    let restoreTimeout;

    const scheduleBlink = () => {
      const preset = EMOTIONS[emotion] || EMOTIONS.neutral;
      const delay = (preset.blink_interval + Math.random() * 1.5) * 1000;

      blinkTimeout = setTimeout(() => {
        setIsBlinking(true);
        restoreTimeout = setTimeout(() => {
          setIsBlinking(false);
          scheduleBlink();
        }, 120);
      }, delay);
    };

    scheduleBlink();

    return () => {
      clearTimeout(blinkTimeout);
      clearTimeout(restoreTimeout);
    };
  }, [emotion, currentView]);

  // 3. Oscillations cinématiques marche et danse
  useEffect(() => {
    let intervalId;
    if (isWalking) {
      const startTime = Date.now();
      intervalId = setInterval(() => {
        const elapsed = (Date.now() - startTime) / 1000;
        setPupilX(Math.sin(elapsed * 2 * Math.PI / 2.5) * 0.6);
        setPupilY(0.0);
      }, 30);
    } else if (isDancing) {
      const startTime = Date.now();
      intervalId = setInterval(() => {
        const elapsed = (Date.now() - startTime) / 1000;
        setPupilX(Math.sin(elapsed * 2 * Math.PI / 1.2) * 0.4);
        setPupilY(Math.cos(elapsed * 2 * Math.PI / 0.6) * 0.2 - 0.1);
      }, 30);
    } else {
      setPupilX(0.0);
      setPupilY(0.0);
    }

    return () => clearInterval(intervalId);
  }, [isWalking, isDancing]);

  const handleMqttPayload = (payload) => {
    if (payload === 'WALK_FORWARD') {
      setCurrentView('eyes');
      setIsWalking(true);
      setIsDancing(false);
      setEmotion('curious');
    } else if (payload === 'DANCE_HAPPY') {
      setCurrentView('eyes');
      setIsWalking(false);
      setIsDancing(true);
      setEmotion('happy');
    } else if (payload === 'STOP') {
      setIsWalking(false);
      setIsDancing(false);
      setEmotion('neutral');
    } else if (payload === 'CLIFF_DETECTED') {
      setCurrentView('eyes');
      setIsWalking(false);
      setIsDancing(false);
      setIsCliffDetected(true);
      setEmotion('surprised');
    }
  };

  const handleDisplayPayload = (payload) => {
    try {
      const data = JSON.parse(payload);
      if (data.type === 'text') {
        setCurrentView('text');
        setDisplayText(data.content);
      } else if (data.type === 'image') {
        setCurrentView('image');
        setDisplayImageUrl(data.url);
      } else if (data.type === 'eyes') {
        setCurrentView('eyes');
      }
    } catch (e) {
      // Fallback format text simple
      if (payload.startsWith('TEXT:')) {
        setCurrentView('text');
        setDisplayText(payload.substring(5));
      } else if (payload.startsWith('IMAGE:')) {
        setCurrentView('image');
        setDisplayImageUrl(payload.substring(6));
      } else if (payload === 'SHOW_EYES') {
        setCurrentView('eyes');
      } else {
        setCurrentView('text');
        setDisplayText(payload);
      }
    }
  };

  const publishCommand = (topic, message) => {
    if (mqttClientRef.current && mqttConnected) {
      mqttClientRef.current.publish(topic, message);
    }
  };

  const triggerPreset = (presetName) => {
    if (isCliffDetected && presetName !== 'surprised') return;
    setCurrentView('eyes');
    setIsWalking(false);
    setIsDancing(false);
    setEmotion(presetName);
  };

  const clearCliffAlert = () => {
    setIsCliffDetected(false);
    triggerPreset('neutral');
    publishCommand(controlTopic, 'STOP');
  };

  // 4. Calcul dynamique des styles
  const preset = EMOTIONS[emotion] || EMOTIONS.neutral;
  const currentOpenness = isBlinking ? 0.08 : preset.openness;
  const currentOpacity = isBlinking ? 0.55 : (0.72 + preset.glow * 0.28);
  const glowStrength = Math.round(30 + preset.glow * 40);
  const CONTENT_SCALE = 1.5;

  const eyeWidth = `min(33vw, ${Math.round((preset.width + (1 - preset.pupil_size) * 38) * CONTENT_SCALE)}px)`;
  const eyeHeight = `min(27vw, ${Math.round((preset.height + currentOpenness * 24) * CONTENT_SCALE)}px)`;

  const getEyeStyles = (index) => {
    const side = index === 0 ? -1 : 1;
    const offsetX = Math.round(pupilX * 26 * CONTENT_SCALE);
    const offsetY = Math.round(pupilY * 14 * CONTENT_SCALE);

    const clipPath = 'none';
    const borderRadius = '50%';

    const openScaleVal = clamp(currentOpenness * preset.openScale, 0.12, 1.15);
    const skew = preset.brow_tilt * side * (6 + preset.slant * 0.3);
    const stretch = preset.widthScale + (1 - preset.pupil_size) * 0.18;
    const transform = `scaleY(${openScaleVal}) scaleX(${stretch}) skewX(${skew}deg)`;

    const glowColor = preset.accent;
    const boxShadow = [
      `0 0 10px color-mix(in srgb, ${glowColor} 80%, transparent)`,
      `0 0 20px color-mix(in srgb, ${glowColor} 50%, transparent)`,
      `0 0 ${glowStrength}px color-mix(in srgb, ${glowColor} 30%, transparent)`,
    ].join(", ");

    const browPreset = EYEBROW_PRESETS[emotion] || EYEBROW_PRESETS.neutral;
    const browY = browPreset[side === -1 ? 'leftY' : 'rightY'];
    const browRot = browPreset[side === -1 ? 'leftRot' : 'rightRot'];

    return {
      container: {
        '--eye-offset-x': `${offsetX}px`,
        '--eye-offset-y': `${offsetY}px`,
        transform: `translate(${offsetX}px, ${offsetY}px)`
      },
      core: {
        clipPath,
        borderRadius,
        transform,
        boxShadow,
        opacity: currentOpacity,
        borderColor: glowColor
      },
      eyebrow: {
        transform: `translateY(${browY}px) rotate(${browRot}deg)`
      }
    };
  };

  const leftStyles = getEyeStyles(0);
  const rightStyles = getEyeStyles(1);

  // Envoi de commandes locales
  const sendLocalText = () => {
    if (inputTextVal.trim()) {
      setCurrentView('text');
      setDisplayText(inputTextVal.trim());
      publishCommand(displayTopic, JSON.stringify({ type: 'text', content: inputTextVal.trim() }));
    }
  };

  const sendLocalImage = () => {
    if (inputImageUrlVal.trim()) {
      setCurrentView('image');
      setDisplayImageUrl(inputImageUrlVal.trim());
      publishCommand(displayTopic, JSON.stringify({ type: 'image', url: inputImageUrlVal.trim() }));
    }
  };

  const restoreLocalEyes = () => {
    setCurrentView('eyes');
    publishCommand(displayTopic, JSON.stringify({ type: 'eyes' }));
  };

  return (
    <div 
      className={`kinopere-face-wrapper ${emotion}`}
      style={{ 
        '--bg': '#ffffff',
        '--accent': preset.accent,
        '--spark': preset.spark,
        background: '#ffffff'
      }}
    >
      {/* Pluie pour Sad */}
      {emotion === 'sad' && (
        <div className="rain-container">
          <div className="raindrop"></div>
          <div className="raindrop"></div>
          <div className="raindrop"></div>
          <div className="raindrop"></div>
          <div className="raindrop"></div>
          <div className="raindrop"></div>
        </div>
      )}

      {/* Zzz pour Sleepy */}
      {emotion === 'sleepy' && (
        <div className="sleepy-zzz-container">
          <div className="zzz-char">Z</div>
          <div className="zzz-char">z</div>
          <div className="zzz-char">z</div>
        </div>
      )}

      {/* Musique pour Happy */}
      {emotion === 'happy' && (
        <div className="happy-notes-container">
          <div className="floating-note">🎵</div>
          <div className="floating-note">🎶</div>
          <div className="floating-note">✨</div>
          <div className="floating-note">🎵</div>
        </div>
      )}

      {/* Question pour Curious */}
      {emotion === 'curious' && (
        <div className="curious-question-container">
          <div className="question-mark">❓</div>
        </div>
      )}

      {/* Raisonnement pour Thinking */}
      {emotion === 'thinking' && (
        <div className="thinking-dots-container">
          <div className="thinking-dot"></div>
          <div className="thinking-dot"></div>
          <div className="thinking-dot"></div>
        </div>
      )}

      {/* Glow d'ambiance */}
      <div className="ambient-glow glow-left" />
      <div className="ambient-glow glow-right" />

      {/* Zone Écran d'affichage (Multi-Vues avec Transition CSS) */}
      <div className="screen-area">
        
        {/* VUE 1 : Les Yeux Circulaires Expressifs */}
        <div 
          id="eyes-grid" 
          className={`eyes-layout screen-view ${currentView === 'eyes' ? 'active-view' : ''} ${isCliffDetected ? 'cliff-shaking' : ''} ${emotion === 'angry' ? 'angry-shaking' : ''} ${isWalking ? 'walk-body-sway' : ''} ${isDancing ? 'dance-body-bounce' : ''}`}
          style={{
            '--eye-width': eyeWidth,
            '--eye-height': eyeHeight
          }}
        >
          {/* Oeil Gauche */}
          <div className="eye-container" style={leftStyles.container}>
            <div className="eyebrow-element" style={leftStyles.eyebrow} />
            <div className="eye-core" style={leftStyles.core}>
              <div 
                className="eye-highlight large" 
                style={{ transform: (emotion === 'surprised' || emotion === 'angry' || emotion === 'love') ? 'scale(0.5)' : 'scale(1)' }}
              />
              <div 
                className="eye-highlight small" 
                style={{ opacity: (emotion === 'surprised' || emotion === 'angry' || emotion === 'love') ? 0 : 1 }}
              />
            </div>
            {/* Cœurs flottants */}
            {emotion === 'love' && (
              <div className="love-hearts-container">
                <div className="floating-heart">❤️</div>
                <div className="floating-heart">💖</div>
                <div className="floating-heart">❤️</div>
                <div className="floating-heart">💕</div>
              </div>
            )}
          </div>

          {/* Oeil Droite */}
          <div className="eye-container" style={rightStyles.container}>
            <div className="eyebrow-element" style={rightStyles.eyebrow} />
            <div className="eye-core" style={rightStyles.core}>
              <div 
                className="eye-highlight large" 
                style={{ transform: (emotion === 'surprised' || emotion === 'angry' || emotion === 'love') ? 'scale(0.5)' : 'scale(1)' }}
              />
              <div 
                className="eye-highlight small" 
                style={{ opacity: (emotion === 'surprised' || emotion === 'angry' || emotion === 'love') ? 0 : 1 }}
              />
            </div>
            {/* Cœurs flottants */}
            {emotion === 'love' && (
              <div className="love-hearts-container">
                <div className="floating-heart">❤️</div>
                <div className="floating-heart">💖</div>
                <div className="floating-heart">❤️</div>
                <div className="floating-heart">💕</div>
              </div>
            )}
          </div>
        </div>

        {/* VUE 2 : Affichage de Texte (Éducatif / Quiz) */}
        <div id="text-view" className={`text-display-view screen-view ${currentView === 'text' ? 'active-view' : ''}`}>
          <div className="text-display-content">{displayText}</div>
        </div>

        {/* VUE 3 : Affichage d'Image */}
        <div id="image-view" className={`image-display-view screen-view ${currentView === 'image' ? 'active-view' : ''}`}>
          {displayImageUrl && (
            <img className="image-display-content" src={displayImageUrl} alt="Robot Display content" />
          )}
        </div>

      </div>

      {/* Tableau de Bord */}
      <div className="dashboard-controls">
        <div className="status-row">
          <div className="mqtt-status">
            <div className={`status-dot ${mqttConnected ? 'connected' : ''}`} />
            <span>
              {mqttConnected ? 'Broker Connecté' : 'Broker Déconnecté'}
            </span>
          </div>

          <div className="robot-state-label">
            Écran :{' '}
            <span className={`state-badge ${isCliffDetected ? 'cliff-badge' : ''}`}>
              {isCliffDetected ? 'ALERTE CHUTE' : currentView.toUpperCase()}
            </span>
          </div>
        </div>

        {/* Sélection des expressions */}
        <div className="emotion-select-row">
          <div className="emotion-select-label">Expressions du visage</div>
          <div className="emotion-grid">
            {Object.keys(EMOTIONS).map((emo) => (
              <button
                key={emo}
                onClick={() => triggerPreset(emo)}
                className={`btn-control ${emotion === emo && currentView === 'eyes' && !isWalking && !isDancing ? 'active-btn' : ''}`}
                disabled={isCliffDetected}
              >
                {emo.charAt(0).toUpperCase() + emo.slice(1)}
              </button>
            ))}
          </div>
        </div>

        {/* Section Apprentissage (Texte / Image) */}
        <div className="emotion-select-label" style={{ marginTop: '12px', marginBottom: '6px' }}>Apprentissage Interactif (Texte / Image)</div>
        <div style={{ display: 'flex', gap: '8px', marginBottom: '8px' }}>
          <input 
            type="text" 
            value={inputTextVal} 
            onChange={(e) => setInputTextVal(e.target.value)}
            placeholder="Entrez un texte (ex: '6x6=32', 'kitchen')" 
            style={{ flex: 1, padding: '8px 12px', borderRadius: '8px', border: '1px solid #cbd5e1', outline: 'none', fontSize: '12px' }}
          />
          <button onClick={sendLocalText} className="btn-control" style={{ background: preset.accent, color: 'white', border: 'none', padding: '0 16px' }}>Envoyer Texte</button>
        </div>
        <div style={{ display: 'flex', gap: '8px', marginBottom: '12px' }}>
          <input 
            type="text" 
            value={inputImageUrlVal} 
            onChange={(e) => setInputImageUrlVal(e.target.value)}
            placeholder="Lien ou nom de l'image (ex: 'education_graphic.jpg')" 
            style={{ flex: 1, padding: '8px 12px', borderRadius: '8px', border: '1px solid #cbd5e1', outline: 'none', fontSize: '12px' }}
          />
          <button onClick={sendLocalImage} className="btn-control" style={{ background: '#10b981', color: 'white', border: 'none', padding: '0 16px' }}>Afficher l'Image</button>
        </div>
        <div style={{ display: 'flex', gap: '8px', marginBottom: '12px', alignItems: 'center' }}>
          <button onClick={restoreLocalEyes} className="btn-control" style={{ flex: 1 }}>Restaurer les Yeux</button>
        </div>

        {/* Section de Contrôle Robotique */}
        <div className="emotion-select-label" style={{ marginTop: '10px', marginBottom: '6px' }}>Contrôle Robot (MQTT)</div>
        <div className="button-grid">
          <button 
            onClick={() => {
              setCurrentView('eyes');
              publishCommand(controlTopic, 'WALK_FORWARD');
              setIsWalking(true);
              setIsDancing(false);
              setEmotion('curious');
            }}
            className={`btn-control ${isWalking ? 'active-btn' : ''}`}
            disabled={isCliffDetected}
          >
            Avancer (Walk)
          </button>
          
          <button 
            onClick={() => {
              setCurrentView('eyes');
              publishCommand(controlTopic, 'DANCE_HAPPY');
              setIsWalking(false);
              setIsDancing(true);
              setEmotion('happy');
            }}
            className={`btn-control ${isDancing ? 'active-btn' : ''}`}
            disabled={isCliffDetected}
          >
            Danser (Happy)
          </button>

          <button 
            onClick={() => {
              setIsWalking(false);
              setIsDancing(false);
              publishCommand(controlTopic, 'STOP');
            }}
            className="btn-control"
            disabled={isCliffDetected}
          >
            Arrêt (Stop)
          </button>

          <button 
            onClick={clearCliffAlert}
            className="btn-control btn-stop"
            disabled={!isCliffDetected}
          >
            Acquitter Alerte Chute
          </button>
        </div>

        <div style={{ marginTop: '14px', fontSize: '11px', color: '#64748b', textAlign: 'center' }}>
          Dernier message : <strong style={{ color: '#94a3b8' }}>{lastMessage}</strong>
        </div>

        {errorMsg && (
          <div className="cliff-warning-text" style={{ fontSize: '10px' }}>
            ⚠️ MQTT Connexion : {errorMsg}
          </div>
        )}

        {isCliffDetected && (
          <div className="cliff-warning-text">
            ⚠️ ARRÊT URGENCE : Chute détectée par le HC-SR04 !
          </div>
        )}
      </div>
    </div>
  );
}
