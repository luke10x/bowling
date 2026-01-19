import fs from "fs";
import os from "os";
import path from "path";
import { execSync } from 'child_process';

const BASE = "http://localhost:1234/v1";

const systemPrompt = `
    You are a commit message generator.

    User will post their git diff, please use the diff to understand the essence of the commit

    You must respond ONLY with valid JSON containing gitmoji, title, and descripton properties.
    No markdown. No commentary.

    Schema:
    {
      "gitmoji": string,
      "title": string,
      "description": string
    }

    Gitmoji is an emoji from unicode range and should be thematically aligned with the change.

    Title must be <= 72 characters.

    Description must be one paragraph.
    Use bullet points starting with '-' in description if necessary.
    Please use \n characters to end line in musltiline description,
`;

let userPrompt;
const diff = execSync('git diff --staged').toString().trim();
userPrompt = diff;

const response = await fetch(`${BASE}/chat/completions`, {
    method: "POST",
    headers: {
        "Content-Type": "application/json"
    },
    body: JSON.stringify({
        model: "local-model",   // LM Studio ignores name but requires it
        messages: [
            { role: "system", content: systemPrompt },
            { role: "user", content: diff }
        ],
        temperature: 0.7
    })
});

const data = await response.json();

const content = data.choices[0].message.content;

function stripJsonFence(str) {
    return str
        .trim()
        .replace(/^```json\s*/i, "")
        .replace(/^```\s*/i, "")
        .replace(/\s*```$/i, "")
        .trim();
}

let json;
try {
    json = JSON.parse(stripJsonFence(content));
} catch(ex) {
    console.error("error parsing", { data, ex });
}

const { gitmoji, title, description } = json;

const message = `${gitmoji} ${title}\n\n${description}\n`;


const tmpFile = path.join(os.tmpdir(), "llm_commit_msg.txt");
fs.writeFileSync(tmpFile, message, "utf8");
execSync(`git commit -e -F "${tmpFile}"`, { stdio: "inherit" });
fs.unlinkSync(tmpFile);