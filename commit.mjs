import fs from "fs";
import os from "os";
import path from "path";
import { execSync } from 'child_process';

const BASE = "http://localhost:1234/v1";

const systemPrompt = `
    You are a commit message generator.

    User will post their git diff, please use the diff to understand the essence of the commit

    You must respond ONLY with valid JSON containing gitmoji, title, and description properties.
    No markdown. No commentary.

    Schema:
    {
      "gitmoji": string,
      "title": string,
      "description": string
    }

    Gitmoji is an emoji from unicode range and should be thematically aligned with the change.

    Title must be <= 72 characters.

    Description must be one paragraph. No longer than 100 words.
    Use bullet points starting with '-' in description if necessary.
    Please use \n characters to end line in multiline description,
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
        model: 'qwen/qwen3-4b-2507',
        messages: [
            { role: "system", content: systemPrompt },
            { role: "user", content: diff }
        ],
        temperature: 0.7
    })
});

const data = await response.json();

let content;
try {
    content = stripJsonFence(data.choices[0].message.content);
} catch (ex) {
    console.error("Error looking for a choice in ", { data });
    process.exit(1);
}

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
    json = JSON.parse(content);
} catch(ex) {
    console.error("error parsing", { data, content, ex });
    process.exit(1);
}

const { gitmoji, title, description } = json;


const message = `${gitmoji} ${title}\n\n${description.substring(1000)}\n`;

// Create temporary file with cat (as requested)
const tmpFile = path.join(os.tmpdir(), "commit_msg.txt");
fs.writeFileSync(tmpFile, message, "utf8");

// Now commit without vim — use --no-edit + -F
execSync(`git commit --no-edit -F "${tmpFile}"`, { stdio: "inherit" });

execSync(`cat "${tmpFile}"`, { stdio: "inherit" });
// Clean up
fs.unlinkSync(tmpFile);
