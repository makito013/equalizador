# Contexto do Projeto — Equalizador

## 1. Visão geral do projeto

**Propósito:** app desktop de uso pessoal (Bruno) para modificar a própria voz em tempo real durante jogos online e tocar efeitos sonoros ("soundboard") audíveis pelos outros participantes, sem sair do jogo.

**Domínio:** processamento de áudio em tempo real / voice changer / soundboard para gaming.

**Plataforma-alvo:** Windows apenas (não há necessidade de macOS/Linux).

**Jogo-alvo principal:** Call of Duty (anti-cheat Ricochet, kernel-level).

**Stack recomendada:** C++ com JUCE (áudio + UI no mesmo framework), WASAPI modo compartilhado como API de áudio padrão.

**Status atual:** primeira rodada de implementação concluída. Scaffold C++/JUCE real no repositório, compilando e rodando no Mac via CoreAudio (ambiente de dev — o alvo final é Windows/WASAPI). Cadeia de áudio completa (Captura→EffectChain→Mixer+Soundboard→Saída) funcional e RT-safe, 184 testes unitários passando, UI mínima (não o design visual completo). Revisor aprovou esta rodada. Detalhes em ADR-004/005 e no log de atualizações.

## 2. Arquitetura

Fluxo de dados: microfone físico → **Capture Layer** (WASAPI) → **DSP/Effects Layer** (voice changer: pitch shift/robô/monstro primeiro; EQ multibanda depois, como mais um nó da cadeia) → **Mixer Layer** (soma voz processada + saída do **Soundboard Layer**, com ganhos independentes) → **Output/Routing Layer** (escreve no endpoint WASAPI de render do driver de áudio virtual) → dispositivo virtual "microfone" selecionável no jogo/Discord.

Camadas transversais:
- **Hotkey/Input Layer** — captura global de teclado (funciona com o jogo em foco), dispara toggle de bypass e reprodução de sons do soundboard via fila lock-free.
- **Bypass/State Layer** — single source of truth do estado ligado/desligado do efeito de voz, lido tanto pelo DSP quanto pela UI (indicador visual).
- **Persistence Layer** — config local em JSON (dispositivos selecionados, efeito/preset ativo, teclas mapeadas, pasta do soundboard, volumes).
- **UI Layer** — seletores de dispositivo, indicador de bypass, botões do soundboard com tecla vinculada. Sem overlay in-game no MVP.
- **Installer/Bootstrap Layer** — embute e instala silenciosamente o driver de áudio virtual (VB-CABLE) durante o setup do app.

**Princípio estrutural chave:** DSP e Mixer não conhecem Windows/WASAPI/drivers — só recebem/entregam buffers PCM. É a fronteira de desacoplamento que permite trocar a estratégia de driver virtual (ou até a API de captura) sem tocar em efeito/mixagem. A camada de Saída trata o endpoint alvo como configurável por nome/ID, nunca hard-coded ao VB-CABLE.

A thread de áudio (Captura→DSP→Mixer→Saída) nunca aloca memória nem toma locks; comunicação com UI/hotkeys é via flags atômicas / estruturas lock-free — requisito de arquitetura, não detalhe de implementação.

## 3. Convenções de código

Estabelecidas na primeira rodada de implementação (Dev):
- Namespace `eq` para todo o código do projeto.
- Contrato de thread-safety documentado no header de cada classe que toca a thread de áudio (o que pode/não pode alocar ou travar).
- Convenção `prepare()` fora da thread de áudio (pode alocar, resolver dispositivos, carregar arquivos) / `process()` (ou equivalente de callback) estritamente RT-safe — sem alocação, sem lock, sem I/O.
- Buffers de trabalho dimensionados uma vez em `prepare()` com `setSize(..., avoidReallocating=true)`; no callback, apenas `jmin(numSamples, capacidadeReservada)`, nunca realoca.
- Camadas de DSP/Mixer/Soundboard/State/Persistence não incluem headers de plataforma (`juce_audio_devices`, `juce_gui_*`) — só `Capture`/`Output`/`Hotkeys`/`MainComponent` tocam plataforma. Validado por grep na revisão.
- Flags de warning do JUCE ativas (ex: `-Wsign-conversion`) tratadas como erro a corrigir, não a silenciar.
- Testes unitários via `juce::UnitTest` (não Catch2) — evita dependência extra já que o código sob teste usa tipos JUCE (`AudioBuffer`, `var`, `File`).

## 4. Decisões importantes e histórico

- **ADR-001 — Dispositivo de áudio virtual via VB-CABLE embutido, instalado silenciosamente pelo instalador do Equalizador.** Decisão motivada por exigência do Bruno de não instalar nada manualmente à parte. Driver próprio do zero foi descartado (exigiria certificado de assinatura EV e homologação com a Microsoft — desproporcional a projeto pessoal). Voicemeeter completo também foi descartado por trazer mixer GUI concorrente com o mixer próprio do Equalizador. Ressalva: VB-CABLE é gratuito para uso pessoal; se o escopo mudar para distribuir o app a terceiros, revisitar licenciamento de redistribuição com a VB-Audio.
- **ADR-002 — C++ com JUCE como framework central de áudio e UI.** Cobre captura, DSP, mixagem e UI num único projeto. Alternativas descartadas: Rust+cpal (ecossistema de efeitos de voz ainda imaturo, exigiria FFI para as mesmas libs C/C++ mesmo assim); .NET/NAudio (risco de stutter na thread de áudio por causa do garbage collector).
- **ADR-003 — Mixagem de voz + soundboard acontece dentro do próprio Equalizador**, não delegada a ferramenta externa (ex: Voicemeeter). Toda a lógica de volume/ganho fica sob controle direto do app.
- **Escopo do "modificar voz":** o Bruno quer voice changer completo (pitch shift, robô, monstro) como prioridade, com EQ de bandas vindo depois — não é só um equalizador simples, apesar do nome do projeto.
- **Soundboard:** apenas atalho de teclado global no MVP, sem overlay visual in-game.
- **Anti-cheat (Ricochet/CoD):** risco avaliado como baixo — drivers de áudio virtual são amplamente usados por streamers/jogadores de CoD sem histórico relevante de banimento (diferente de injeção de processo/leitura de memória, que é o alvo real de anti-cheat). Não é motivo para mudar de abordagem, mas vale validar empiricamente quando o app existir.
- **ADR-004 — Pitch-shift via SoundTouch (fechado pelo TL).** Fonte canônica com tags versionadas: `codeberg.org/soundtouch/soundtouch` 2.4.1 — o mirror `github.com/soundtouch/soundtouch` não expõe tags. Rubber Band fica como plano B só se a qualidade do efeito "monstro" não for suficiente após medição, não por preferência a priori. Licença LGPL: build de dev local usa SoundTouch estático (aceitável para uso pessoal), mas o **release Windows precisa linkar dinamicamente** (ou distribuir objetos relinkáveis) para conformidade — vira item obrigatório do checklist de empacotamento.
- **Decisões de UI do Designer (etapa 5, primeira rodada de código):** tema escuro fixo, sem opção de tema claro no MVP (app fica em segundo plano/segundo monitor ao lado do jogo). Soundboard em grid fixo sem scroll (se precisar de mais sons no futuro, usar abas fixas, não lista rolável) — isso restringe como a estrutura de dados dos sons deve ser modelada (lista limitada, não paginação). Vermelho reservado exclusivamente para estado de erro real ("sem sinal de microfone"), nunca para bypass/voz normal. "Ouvir a mim mesmo" é um checkbox simples, não um segundo seletor de dispositivo de saída — mas o TL registrou que isso implica uma stream WASAPI secundária (`SelfMonitorOutput`) com clock próprio, não é "de graça" tecnicamente (ver seção 6).
- **Ambiente de desenvolvimento é macOS, alvo final é Windows.** Decisão do Bruno: implementação usa `juce::AudioDeviceManager` (abstrai CoreAudio/WASAPI) para que Capture/DSP/Mixer/Soundboard/Persistence/UI compilem e rodem de verdade no Mac durante o desenvolvimento; apenas o que é genuinamente Windows-only (hotkeys globais via `RegisterHotKey`, detecção do VB-CABLE) fica isolado atrás de interface (`IHotkeyBackend`) e só será compilado/testado numa máquina Windows real.

## 5. Integrações externas / dependências entre projetos

- **VB-CABLE (VB-Audio)** — driver de áudio virtual de terceiro, embutido no instalador do Equalizador e instalado silenciosamente. Gratuito para uso pessoal (donationware); redistribuição para terceiros exigiria consulta à VB-Audio. Detecção real (`VBCableDetector`) ainda é stub — depende de enumeração de endpoints Windows-specific, não implementada nesta rodada.
- **SoundTouch** (pitch-shift) — decidido, ver ADR-004. Fonte: `codeberg.org/soundtouch/soundtouch` 2.4.1.

## 6. Áreas sensíveis / gotchas conhecidos

- **Thread-safety em tempo real:** a thread de áudio não pode alocar memória nem usar locks — qualquer bug aqui vira glitch audível ou crash durante uma partida.
- **Latência:** meta é <50-100ms ponta a ponta; precisa de tuning empírico de buffer WASAPI na máquina real do Bruno. ASIO fica em aberto só se o modo compartilhado não bastar.
- **Instalação/desinstalação do VB-CABLE:** lidar com driver já instalado previamente na máquina, versionamento e desinstalação limpa junto com o Equalizador.
- **Dependência de política de assinatura de driver da Microsoft** mudar no futuro — fora do controle do projeto; mitigado parcialmente pelo desacoplamento do endpoint de saída (ADR-001).
- **Hotkeys globais em fullscreen exclusivo:** recomenda-se orientar o uso de "Borderless Windowed" no CoD para máxima compatibilidade; validar `RegisterHotKey` empiricamente antes de considerar RF07/RF09 resolvidos.
- **Escolha do algoritmo de pitch-shift** (qualidade vs. custo de CPU) precisa de prototipagem, especialmente rodando ao lado do CoD. TL registrou risco real: SoundTouch mal tunado pode sozinho consumir boa parte do orçamento de latência de 50-100ms — spike de medição é a primeira tarefa técnica antes de investir em UI/efeitos adicionais.
- **`PitchShifter`/SoundTouch não é comprovadamente RT-safe** — o FIFO interno da lib pode alocar; há warm-up de blocos de silêncio no `prepare()` para estabilizar, mas é garantia empírica, não formal. Precisa validação com profiler (ex: xrun counter) na máquina Windows real antes de considerar o DSP fechado.
- **Race latente em `SoundboardEngine`:** `loadSlot()` troca o buffer do slot sem sincronização com a thread de áudio que lê em `renderNextBlock()`. Não é exercitada nesta rodada (nenhum caminho de código chama `loadSlot` ainda), mas precisa de double-buffering ou troca atômica de ponteiro antes de suportar reload de som em tempo real.
- **`OutputRouter`/`AudioCaptureSource` truncam silenciosamente** se o driver mudar o buffer size em runtime sem novo ciclo de `prepare` — risco teórico no CoreAudio do Mac, mas relevante ao migrar para WASAPI real (shared/exclusive mode podem trocar buffer size).
- **Auto-monitoração ("ouvir a mim mesmo") ainda não implementada** (`SelfMonitorOutput`) — exige duas streams WASAPI simultâneas com clocks independentes (drift em sessões longas), tratado como módulo próprio no plano do TL, não uma extensão trivial do `OutputRouter`.

## 7. Log de atualizações

- **2026-07-12** — Criação inicial do contexto via pipeline `/orquestrador` (etapas Analista → PO → Arquiteto). Origem: `pipeline`. Consolidado a partir da primeira sessão de planejamento do projeto: requisitos levantados, MVP definido, arquitetura macro e ADRs de dispositivo de áudio virtual / pilha técnica / mixagem definidos. Nenhum código implementado ainda.
- **2026-07-13** — Primeira rodada de implementação via pipeline `/orquestrador` (etapas Designer → TL → Dev → Revisor). Origem: `pipeline`. Designer definiu UI (tema escuro, layout, indicador de bypass, wizard); TL fechou SoundTouch como lib de pitch-shift e entregou plano de 15 tarefas (~22-29 dias) com estrutura de módulos C++/JUCE; Dev implementou scaffold real (CMake+JUCE+SoundTouch), cadeia de áudio completa e RT-safe, hotkeys lock-free, mixer, soundboard, persistência JSON e UI mínima, compilando e rodando no Mac via CoreAudio (ambiente de dev; alvo final Windows/WASAPI) — 184 testes unitários passando; Revisor leu o código, rodou os binários e testes pessoalmente, e aprovou (✅) com 3 itens de dívida técnica não-bloqueantes registrados acima. Fora de escopo desta rodada, deliberadamente: `SelfMonitorOutput`, `VBCableDetector`/wizard real, `Win32HotkeyBackend` testado em Windows real, UI visual completa do Designer.
