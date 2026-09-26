<p align="center">
  <img src="https://community.trinitycore.org/public/style_images/1_trinitycore.png" alt="TrinityCore Logo" width="200"/>
</p>

<h1 align="center">⚔️ TRIBOTSLK</h1>

<p align="center">
  <em>TrinityCore 3.3.5a con NPCBots inteligentes que conversan usando IA</em>
</p>

<p align="center">
  <a href="https://github.com/Razormaw/TRIBOTSLK/stargazers">
    <img src="https://img.shields.io/github/stars/Razormaw/TRIBOTSLK?style=for-the-badge&color=gold&logo=starship" alt="Stars"/>
  </a>
  <a href="https://github.com/Razormaw/TRIBOTSLK/blob/main/COPYING">
    <img src="https://img.shields.io/badge/license-GPL--2.0-blue?style=for-the-badge&logo=gnu" alt="License"/>
  </a>
  <a href="#">
    <img src="https://img.shields.io/badge/WoW-3.3.5a-orange?style=for-the-badge&logo=blizzard" alt="WoW"/>
  </a>
  <a href="#">
    <img src="https://img.shields.io/badge/NPCBots-5.2.77a-purple?style=for-the-badge" alt="NPCBots"/>
  </a>
  <a href="https://chat.qwen.ai/">
    <img src="https://img.shields.io/badge/LLM-Qwen%203.8%2027B-00d4aa?style=for-the-badge&logo=openai" alt="LLM"/>
  </a>
</p>

<p align="center">
  <a href="#-sobre-el-proyecto">Sobre el proyecto</a> •
  <a href="#-características">Características</a> •
  <a href="#-instalación">Instalación</a> •
  <a href="#-agradecimientos">Agradecimientos</a> •
  <a href="#-licencia">Licencia</a>
</p>

---

## 📖 Sobre el proyecto

**TRIBOTSLK** es un fork extendido de [TrinityCore 3.3.5a](https://github.com/TrinityCore/TrinityCore) con el mod [NPCBots](https://github.com/trickerer/TrinityCore-3.3.5-with-NPCBots), que agrega una capa única de **inteligencia artificial conversacional** sobre el sistema de bots existente.

> 🌟 **Nació de una motivación simple:** *los bugs que me frustraban como jugador, hoy los arreglo como desarrollador.*

### ¿Qué hace especial a este proyecto?

✨ **180+ NPCBots** vagando por Azeroth con conversaciones espontáneas  (Gracias a Trickerer)
🤖 **Chat con IA** usando Qwen 3.8 27B vía Groq (respuesta en 2-4 segundos)  
💬 **Cadenas de respuestas**: varios bots te contestan en fila, como una mesa de taberna  
🌍 **Mundo vivo**: los bots charlan entre ellos cuando no hay jugadores  
🎭 **Personalidades distintas** según raza y clase de cada bot  
🔄 **Failover automático** a base de datos si la IA falla  
⚡ **Arquitectura asíncrona**: el servidor nunca se bloquea

---

## ✨ Características

### 🤖 Sistema de Chat LLM para NPCBots

| Funcionalidad | Descripción |
|:--|:--|
| 💬 **Respuestas a jugadores** | 2-3 bots responden en cadena con pausas naturales de 2-4 s |
| 🗣️ **Charlas bot-a-bot** | Conversaciones espontáneas cada ~2.5 minutos |
| 📻 **Canales configurables** | Escuchan en cualquier canal del `worldserver.conf` |
| 👤 **Nombres visibles** | Los bots aparecen con nombre en el chat del cliente |
| 🧠 **Contexto multi-turno** | Los bots se citan entre ellos |
| 🛡️ **Failover inteligente** | Si la API falla, usan frases de la BD |

### 💭 Ejemplo de conversación en el canal `world`

```text
[Galcynd]: Hola a todos
[Kym]: Hola, viajero. Que los dioses te cuiden en los Reinos.
[Maxan]: Saludo, Kym. Que tus oraciones salven a Malfurion.
[Azar]: Azar escucha en silencio, y un murmullo de viento responde a la oración.

[Galcynd]: uff, muy romanticos están hoy
[Tani]: Bien se nota. El amor flota en los aires de Azeroth hoy.
[Rothik]: El amor? Dudo que sobreviva al filo de mis espadas.
```

### 🏗️ Características técnicas

- Arquitectura asíncrona con **hilos separados** para llamadas HTTP
- **GUID virtual de jugador** para mostrar nombres en el canal
- Paquetes de red personalizados (`SMSG_NAME_QUERY_RESPONSE` + `SMSG_MESSAGECHAT`)
- Tabla `npcbot_llm_phrases` editable en caliente (sin recompilar)
- Configuración 100% flexible vía `worldserver.conf`

---

## 📋 Requisitos

### Software base

Consulta la [wiki oficial de TrinityCore](https://trinitycore.info/en/install/requirements) para Windows, Linux y macOS.

### Requisitos adicionales para el Chat LLM

- 🔑 **API Key de Groq**: gratuita en [console.groq.com](https://console.groq.com/)
- 🤖 **Modelo recomendado**: `qwen/qwen3.8-27b` (compatible con OpenAI API)
- 🌐 **Conexión a Internet**: el servidor necesita acceso a `api.groq.com`

---

## 🚀 Instalación

### 1️⃣ Clonar el repositorio

```bash
git clone https://github.com/Razormaw/TRIBOTSLK.git
cd TRIBOTSLK
```

### 2️⃣ Compilar TrinityCore

Sigue la [guía oficial](https://trinitycore.info/en/home) y respeta las [opciones de NPCBots](https://github.com/trickerer/Trinity-Bots#npcbot-mod-installation).

### 3️⃣ Importar la tabla de frases

Ejecuta en tu base de datos **world**:

```sql
CREATE TABLE IF NOT EXISTS `npcbot_llm_phrases` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `category` VARCHAR(32) NOT NULL DEFAULT 'inicio',
  `text` VARCHAR(255) NOT NULL,
  PRIMARY KEY (`id`),
  KEY `idx_category` (`category`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

INSERT INTO `npcbot_llm_phrases` (`category`, `text`) VALUES
('inicio', 'Alguien vio jinetes de la Horda cerca del lago esta mañana?'),
('inicio', 'Dicen que en Ventormenta subieron el precio de la cerveza otra vez.'),
('inicio', 'Alg sanador en el canal? Mi grupo necesita uno para Zul\'Farrak.'),
('respuesta', 'Yo no me metería ahí ni por todo el oro de Lunargenta.'),
('respuesta', 'Suena a problema tuyo, amigo. Yo solo paso por la cerveza.'),
('respuesta', 'Mis ojos han visto cosas peores en las Estepas.');
```

### 4️⃣ Configurar el sistema LLM

Agrega este bloque a tu `worldserver.conf`:

```ini
# =====================================================
# SISTEMA LLM PARA NPCBOTS
# =====================================================
NpcBot.LLM.Enable = 1
NpcBot.LLM.ApiKey = "tu_api_key_de_groq_aqui"
NpcBot.LLM.Model = "qwen/qwen3.8-27b"
NpcBot.LLM.Channels = "world,mundo,general,taberna"

# Charlas bot-a-bot
NpcBot.LLM.BotToBot.Enable = 1
NpcBot.LLM.BotToBot.CooldownSec = 150
NpcBot.LLM.BotToBot.Chance = 25

# Cadena de respuestas a jugadores
NpcBot.LLM.PlayerChain.Enable = 1
NpcBot.LLM.PlayerChain.MinTurns = 2
NpcBot.LLM.PlayerChain.MaxTurns = 3
```

### 5️⃣ Iniciar el servidor

```bash
./worldserver
```

¡Listo! Después de ~2 minutos los bots comenzarán a conversar. 🎉

---

## 🎮 Uso en el juego

1. Únete al canal: `/join world`
2. Escribe algo: `Hola a todos`
3. Los bots responderán en cadena con pausas naturales

> 💡 Si no hay actividad de jugadores por ~2.5 minutos, los bots comenzarán a charlar entre ellos espontáneamente. ¡Observa la magia! ✨

---

## 🐛 Reportar problemas

Antes de abrir un issue, revisa los [issues existentes](https://github.com/Razormaw/TRIBOTSLK/issues) para evitar duplicados.

Al reportar, incluye:
- Descripción clara del problema
- Pasos para reproducirlo
- Logs relevantes del servidor
- Capturas de pantalla si aplica

---

## 🤝 Contribuir

Las contribuciones son bienvenidas. Para enviar fixes:

```bash
git checkout -b feature/mi-arreglo
git commit -am 'Agrega nueva funcionalidad'
git push origin feature/mi-arreglo
# Abre un Pull Request en GitHub
```

---

## 💖 Agradecimientos

> *Este proyecto no existiría sin el trabajo desinteresado de muchas personas y comunidades. Mi más sincero agradecimiento a todos ellos.*

### 🏛️ A los gigantes sobre cuyos hombros caminamos

<table>
<tr>
<td width="50%" valign="top">

#### 🏰 [TrinityCore](https://github.com/TrinityCore/TrinityCore)

El **core del emulador**. Años de desarrollo desinteresado, documentación exhaustiva y una comunidad increíble. Sin TrinityCore, este proyecto simplemente no existiría.

*Gracias por mantener vivo el espíritu del código abierto.* 🙏

</td>
<td width="50%" valign="top">

#### 🤖 [Trickerer — NPCBots](https://github.com/trickerer/TrinityCore-3.3.5-with-NPCBots)

El **sistema de bots** que hace posible tener 180+ NPCBots vagando por Azeroth. Tu trabajo es la base sobre la que construí todo esto.

*Un mod que cambió la forma de jugar WoW privado.* ⚔️

</td>
</tr>
</table>

### 🧠 A la inteligencia que da vida a los bots

<table>
<tr>
<td width="50%" valign="top">

#### 🤖 [Qwen AI](https://chat.qwen.ai/)

El **cerebro de las conversaciones**. Qwen 3.8 27B genera texto en español, en personaje y con contexto, haciendo que cada conversación se sienta natural y única.

*La magia detrás de cada réplica.* ✨

</td>
<td width="50%" valign="top">

#### ⚡ [Groq](https://groq.com/)

La **velocidad de inferencia**. Acceso gratuito a modelos de IA con latencia increíble: respuestas en 2-4 segundos en lugar de 10+.

*Por hacer que la IA sea accesible para todos.* 🚀

</td>
</tr>
</table>

### 👨‍👩‍👧‍👦 A mi familia — mi motor y mi destino

<div align="center">

#### 💕 A mi esposa Ana María

> Por tu paciencia infinita durante las noches de debugging.
> Por traerme café a las 3 AM.
> Por creer en este proyecto incluso cuando yo dudaba.
>
> Eres mi roca y mi motivación.
> **Te amo más que a cualquier línea de código.** 💖

</div>

<div align="center">

#### 🎮 A mis hijos Luis Angel, Santos Alejandro y José Francisco

> Porque Luis y Alejandro juegan WoW desde que pudieron hacerlo despues de nacer (aunque no sabían caminar aun). 💖
> Por ser la razón por la que hago esto.
> Ver sus caras cuando los bots les respondieron por primera vez.
> fue el mejor momento de todo el proyecto.
>Por José Francisco que ojal'a alg'un día pueda jugar WoW. 💖
> Ustedes me recuerdan por qué los videojuegos son mágicos.
> **Este servidor es para ustedes.** 🌟

</div>

### 🌟 A la comunidad

- A la **comunidad de emuladores de WoW**: por los foros, wikis y herramientas.
- A mis **amigos beta testers**: por probar, reportar y darme feedback.
- A **todos los que clonen este repo**: por mantener vivo el espíritu de la experimentación.
- A los **asistentes de IA** (Qwen y compañeras): herramientas, no autoras. El código y las pruebas son del desarrollador. 🛠️

---

## 📜 Licencia

Este proyecto está licenciado bajo la **GNU General Public License v2.0** — ver el archivo [`COPYING`](COPYING).

> ⚠️ **Nota importante:** Todo el crédito del core de TrinityCore y del sistema de NPCBots pertenece a sus respectivos autores. Este proyecto agrega una capa de funcionalidad (chat LLM) sobre su trabajo, sin modificar la arquitectura base.

---

## 🔗 Enlaces útiles

| Recurso | Enlace |
|:--|:--|
| 🏰 TrinityCore Website | [trinitycore.org](https://www.trinitycore.org) |
| 📚 TrinityCore Wiki | [trinitycore.info](https://www.trinitycore.info) |
| 💬 TrinityCore Forums | [talk.trinitycore.org](https://talk.trinitycore.org/) |
| 🎮 TrinityCore Discord | [discord.trinitycore.org](https://discord.trinitycore.org/) |
| 🤖 NPCBots (trickerer) | [github.com/trickerer/Trinity-Bots](https://github.com/trickerer/Trinity-Bots) |
| ⚡ Groq Cloud | [console.groq.com](https://console.groq.com/) |
| 🧠 Qwen AI | [chat.qwen.ai](https://chat.qwen.ai/) |

---

<div align="center">

### 🍻 Hecho con 💖, ☕ y muchas noches sin dormir

*Que los bots de Azeroth tengan conversaciones inteligentes.*

**`-- Razormaw, TRIBOTSLK --`**

<img src="https://img.shields.io/badge/Made%20with-❤️-red?style=flat-square"/>
<img src="https://img.shields.io/badge/Powered%20by-AI-00d4aa?style=flat-square"/>
<img src="https://img.shields.io/badge/Built%20for-Family-gold?style=flat-square"/>

</div>