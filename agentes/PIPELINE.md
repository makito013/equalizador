# Pipeline de Desenvolvimento (ciclo completo)

O Orquestrador gerencia um pipeline configurável. Você escolhe quais etapas ativar por sessão.

```
[1] ANALISTA → [2] PO → [3] ARQUITETO → [4] BDD → [5] DESIGNER
                                                          ↓
                                                      [6] TL
                                                          ↓
                                                      [7] DEV
                                                          ↓
                                                      [8] QA ─── falhou ──→ volta ao DEV
                                                          ↓
                                                     [9] REVISOR ── reprovado ──→ volta ao DEV
                                                          ↓
                                                   [10] SEGURANÇA ── bloqueado ──→ volta ao DEV
                                                          ↓
                                                     ✅ FEITO
```

## Todos os Agentes

| # | Etapa | Agente | Arquivo | Obrigatório? |
|---|-------|--------|---------|--------------|
| 1 | Análise da solicitação | Analista | `agentes/ANALISTA.md` | Sempre |
| 2 | Clarificação de requisitos | PO | `agentes/PO.md` | Recomendado |
| 3 | Planejamento de arquitetura | Arquiteto | `agentes/ARQUITETO.md` | Recomendado |
| 4 | Cenários de comportamento | BDD | `agentes/BDD.md` | Opcional |
| 5 | Design de interface | Designer | `agentes/DESIGNER.md` | Opcional |
| 6 | Plano técnico + estratégia de testes | TL | `agentes/TL.md` | Recomendado |
| 7 | Implementação do código | Dev | `agentes/DEV.md` | Sempre |
| 8 | Testes unitários e integração | QA | `agentes/QA.md` | Opcional |
| 9 | Revisão: pedido vs. entregado | Revisor | `agentes/REVISOR.md` | Recomendado |
| 10 | Auditoria de segurança | Segurança | `agentes/SEGURANCA.md` | Opcional |
| — | Orquestração do pipeline | Orquestrador | `agentes/ORQUESTRADOR.md` | Sempre ativo |

## Perfis rápidos de pipeline

| Perfil | Etapas ativas |
|--------|--------------|
| 🏃 Projeto pessoal/protótipo | 1 → 7 → 9 |
| 🔧 Feature simples | 1 → 2 → 6 → 7 → 9 |
| 🏗️ Feature com UI | 1 → 2 → 3 → 5 → 6 → 7 → 9 |
| 🧪 Produção com testes | 1 → 2 → 3 → 4 → 6 → 7 → 8 → 9 |
| 🔒 Produção completa | todas (1 ao 10) |
| 🐛 Bug simples | 1 → 7 → 9 |
| 🔍 Bug complexo | 1 → 6 → 7 → 8 → 9 |
| 🔐 Bug de segurança | 1 → 6 → 7 → 8 → 9 → 10 |

## Template de TEAM.md

Se `agentes/TEAM.md` existir no projeto, ele define a pré-seleção do menu de
`/orquestrador` (o Bruno ainda pode ajustar por sessão). Formato:

```
# Time padrão — <projeto>

Define a pré-seleção do menu quando /orquestrador rodar aqui.
O Bruno ainda pode ajustar por sessão — isto só muda o ponto de partida.

[x] 1. ANÁLISE — Analista
[ ] 2. CLARIFICAÇÃO — PO
[x] 3. ARQUITETURA — Arquiteto
[ ] 4. BDD
[ ] 5. UX/UI — Designer
[x] 6. TECH LEAD — TL
[x] 7. DESENVOLVIMENTO — Dev (sempre ativo, não editável)
[ ] 8. TESTES UNITÁRIOS — QA
[x] 9. REVISÃO — Revisor
[ ] 10. SEGURANÇA
```

A etapa 7 (Desenvolvimento) nunca pode ficar desmarcada — `/orquestrador-team`
recusa a edição se o Bruno tentar desativá-la.

## Template de CONTEXTO.md

`agentes/CONTEXTO.md` é a memória persistente de um projeto. Gerado/atualizado
por `/orquestrador-init` e realimentado durante o uso normal do pipeline
(ver "Como disparar cada etapa" em `ORQUESTRADOR.md`). Sempre com estas 7
seções, nesta ordem:

1. **Visão geral do projeto** — propósito, domínio, stack.
2. **Arquitetura** — camadas, padrões, decisões estruturais.
3. **Convenções de código** — estilo, nomenclatura, padrões observados no repo.
4. **Decisões importantes e histórico** — por que certas escolhas foram feitas.
5. **Integrações externas / dependências entre projetos** — ex: "consome os
   endpoints X e Y do serviço `ymci-backend`; contrato em `docs/api/...`".
   Existe para o caso de monorepo onde um projeto secundário depende de 1-2
   endpoints do produto principal, sem precisar importar o contexto inteiro
   do outro projeto.
6. **Áreas sensíveis / gotchas conhecidos** — coisas que quebram fácil, dívida
   técnica.
7. **Log de atualizações** — data, o que mudou, origem (`init` ou `pipeline`).

Ao fundir com um `CONTEXTO.md` já existente: preserva o que ainda é válido,
atualiza o que mudou, sempre registra uma linha nova na seção 7.

## Como usar

**Inicie sempre pelo Orquestrador (Claude Code):**
> `/orquestrador quero adicionar login com Google ao projeto`

O Orquestrador vai:
1. Confirmar o que entendeu
2. Apresentar o menu de etapas
3. Você marca quais quer ativar
4. Ele dispara os agentes na ordem e traz os resultados

## Subagentes e escolha de modelo

Qualquer agente deste pipeline (inclusive o Orquestrador) pode disparar
subagentes próprios para paralelizar partes independentes do seu próprio
trabalho.

- Modelo padrão: Sonnet. Escale para Opus quando perceber complexidade real
  (refatoração ampla, lógica ambígua exigindo raciocínio profundo, código
  security-sensitive, ou quando um subagente Sonnet já não deu conta).
- **Ressalva:** o override de modelo não funciona ao disparar um *fork* — só
  ao disparar um subagente novo (`subagent_type` diferente de fork). Um fork
  sempre roda no modelo de quem o disparou. A escalação pra Opus só vale para
  subagentes "frescos".
