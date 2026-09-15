import { tool } from "@opencode-ai/plugin"

async function runAgy(
  agent: string,
  model: string,
  prompt: string,
  directory: string,
) {
  const proc = Bun.spawn({
    cmd: [
      "agy",
      "--agent", agent,
      "--model", model,
      "--input-format", "stream-json",
      "--output-format", "stream-json",
      "--print-timeout", "10m",
    ],
    cwd: directory,
    stdin: "pipe",
    stdout: "pipe",
    stderr: "pipe",
  })

  const message = {
    event: "user",
    message: {
      content: prompt,
    },
  }

  proc.stdin.write(JSON.stringify(message) + "\n")
  proc.stdin.end()

  const [stdout, stderr, exitCode] = await Promise.all([
    new Response(proc.stdout).text(),
    new Response(proc.stderr).text(),
    proc.exited,
  ])

  let result: any = null

  for (const line of stdout.split(/\r?\n/)) {
    if (!line.trim()) continue

    try {
      const event = JSON.parse(line)

      if (event.event === "result") {
        result = event.result
      }
    } catch {
      // Ignore non-JSON diagnostic lines.
    }
  }

  if (!result) {
    throw new Error(
      `AGY returned no result.\nExit: ${exitCode}\n${stderr}`.trim(),
    )
  }

  if (result.status !== "SUCCESS") {
    throw new Error(
      `AGY failed: ${result.error ?? result.status}\n${stderr}`.trim(),
    )
  }

  const usage = result.usage ?? {}

  return [
    result.response.trim(),
    "",
    "---",
    `AGY: ${agent} / ${model}`,
    `Input: ${usage.input_tokens ?? "?"}`,
    `Output: ${usage.output_tokens ?? "?"}`,
    `Thinking: ${usage.thinking_tokens ?? "?"}`,
    `Total: ${usage.total_tokens ?? "?"}`,
  ].join("\n")
}

export const plan = tool({
  description:
    "Ask the low-cost Gemini Flash planning worker for a compact implementation plan. Give it only the relevant context, never the whole repository.",
  args: {
    prompt: tool.schema.string().describe(
      "Compact task/context packet. Include only information relevant to the decision.",
    ),
  },

  async execute(args, context) {
    return runAgy(
      "flash-plan",
      "gemini-3.8-flash-low",
      args.prompt,
      context.directory,
    )
  },
})

export const code = tool({
  description:
    "Ask the Gemini Flash coding worker to produce precise code changes from a task, approved plan, and relevant source snippets. Do not send the whole repository.",
  args: {
    prompt: tool.schema.string().describe(
      "Task + approved plan + only relevant source code/context.",
    ),
  },

  async execute(args, context) {
    return runAgy(
      "flash-coder",
      "gemini-3.8-flash-medium",
      args.prompt,
      context.directory,
    )
  },
})

export const review = tool({
  description:
    "Ask the low-cost Gemini Flash reviewer to inspect a diff and test results. Send the task, plan, diff, and relevant test output only.",
  args: {
    prompt: tool.schema.string().describe(
      "Original task + plan + resulting diff + relevant test/build output.",
    ),
  },

  async execute(args, context) {
    return runAgy(
      "flash-reviewer",
      "gemini-3.8-flash-low",
      args.prompt,
      context.directory,
    )
  },
})