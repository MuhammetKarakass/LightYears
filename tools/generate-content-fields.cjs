// Run deliberately when extending the game's authoring vocabulary. This is a
// source-generation tool, never a runtime scan or an automatic build step.
const fs = require('fs');
const types = { ability: 'abilities', effect: 'effects', module: 'modules', ship: 'ships', weapon: 'weapons' };
let output = '// Authored field vocabulary. Wildcards describe array rows; unknown keys remain opaque.\n';
for (const [type, collection] of Object.entries(types))
{
	const root = JSON.parse(fs.readFileSync(`LightYearsGame/assets/content/data/${collection}.json`, 'utf8'));
	const fields = new Map();
	function visit(value, path)
	{
		if (!fields.has(path)) fields.set(path, { value, curve: false });
		const field = fields.get(path);
		if (Array.isArray(value))
		{
			if (value.length && value.every(item => typeof item === 'number') && !/(?:color|offset|origin|scale|size|shape|rotation|cells|accentColor)$/i.test(path)) field.curve = true;
			value.forEach(item => visit(item, path + '/*'));
		}
		else if (value && typeof value === 'object')
		{
			Object.entries(value).forEach(([key, item]) => visit(item, path + '/' + key.replace(/~/g, '~0').replace(/\//g, '~1')));
		}
	}
	root[collection].forEach(value => Object.entries(value).forEach(([key, item]) => visit(item, '/' + key)));
	for (const [path, { value, curve }] of fields)
	{
		const defaultValue = Array.isArray(value) ? [] : value && typeof value === 'object' ? {} : value;
		output += `\t\tif (schema.type == "${type}") Add(schema, "${path}", R"field(${JSON.stringify(defaultValue)})field"${curve ? ', true' : ''});\n`;
	}
}
fs.writeFileSync('LightYearsGame/src/gameplay/content/GameContentFields.inc', output);
