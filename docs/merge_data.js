// Merge all agent*_data.js files into APIReference.html
const fs = require('fs');
const path = require('path');

const docsDir = __dirname;
const htmlFile = path.join(docsDir, 'APIReference.html');

// Data files in order
const dataFiles = [
  'agent1_data.js',
  'agent2_data.js',
  'agent3_data.js',
  'agent4a_data.js',
  'agent4b_data.js',
  'agent4c_data.js',
  'agent5_data.js',
  'agent6_data.js',
  'agent7_data.js',
  'agent8a_data.js',
  'agent8b_data.js',
  'agent8c_data.js',
];

// Extract inner content from each data file (between first { and last })
function extractInnerContent(filePath) {
  const content = fs.readFileSync(filePath, 'utf-8');

  // Find first '{'
  const firstBrace = content.indexOf('{');
  if (firstBrace === -1) {
    console.error(`No opening brace found in ${filePath}`);
    return '';
  }

  // Find last '}'
  const lastBrace = content.lastIndexOf('}');
  if (lastBrace === -1 || lastBrace <= firstBrace) {
    console.error(`No closing brace found in ${filePath}`);
    return '';
  }

  // Extract content between braces (exclusive)
  let inner = content.substring(firstBrace + 1, lastBrace).trim();

  // Remove trailing comma if present
  if (inner.endsWith(',')) {
    inner = inner.slice(0, -1).trimEnd();
  }

  return inner;
}

// Read HTML
let html = fs.readFileSync(htmlFile, 'utf-8');

// Find the existing const D={...}; block
// It starts with "const D={" and ends with the matching "};"
// We know from inspection it's at line 2547 and the closing is before "document.addEventListener"
const dStart = html.indexOf('const D={');
if (dStart === -1) {
  console.error('Could not find "const D={" in APIReference.html');
  process.exit(1);
}

// Find the matching closing "};' followed by newline and "document.addEventListener"
const domContentLoaded = html.indexOf("document.addEventListener('DOMContentLoaded'");
if (domContentLoaded === -1) {
  console.error('Could not find DOMContentLoaded in APIReference.html');
  process.exit(1);
}

// The "};" should be just before DOMContentLoaded (possibly with whitespace)
// Search backwards from domContentLoaded for "};"
let dEnd = html.lastIndexOf('};', domContentLoaded);
if (dEnd === -1 || dEnd < dStart) {
  console.error('Could not find closing "};" for const D');
  process.exit(1);
}
dEnd += 2; // include the "};"

console.log(`Found const D block: chars ${dStart} to ${dEnd} (${dEnd - dStart} chars)`);

// Extract and merge all data files
let allEntries = [];
let totalCount = 0;

for (const file of dataFiles) {
  const filePath = path.join(docsDir, file);
  if (!fs.existsSync(filePath)) {
    console.warn(`WARNING: ${file} not found, skipping`);
    continue;
  }

  const inner = extractInnerContent(filePath);
  if (inner) {
    // Count entries (lines matching 'SomeKey-SomeName': [)
    const entryCount = (inner.match(/'[A-Za-z0-9_]+-[A-Za-z0-9_~]+'\s*:\s*\[/g) || []).length;
    console.log(`${file}: ${entryCount} entries`);
    totalCount += entryCount;
    allEntries.push(inner);
  }
}

console.log(`\nTotal entries: ${totalCount}`);

// Build merged D object
const mergedD = 'const D={\n' + allEntries.join(',\n\n') + '\n};';

// Replace in HTML
const newHtml = html.substring(0, dStart) + mergedD + html.substring(dEnd);

// Write output
fs.writeFileSync(htmlFile, newHtml, 'utf-8');

console.log(`\nMerged ${dataFiles.length} data files into APIReference.html`);
console.log(`HTML size: ${(newHtml.length / 1024).toFixed(1)} KB`);
