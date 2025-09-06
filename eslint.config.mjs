import tseslint from '@typescript-eslint/eslint-plugin';
import tsparser from '@typescript-eslint/parser';
import react from 'eslint-plugin-react';
import reactHooks from 'eslint-plugin-react-hooks';
import importPlugin from 'eslint-plugin-import';
import unusedImports from 'eslint-plugin-unused-imports';

export default [
    {
        ignores: ['dist/**'],
    },

    // Only lint files in the frontend directory
    {
        files: ['frontend/**/*.{ts,tsx}'],
        languageOptions: {
            parser: tsparser,
            parserOptions: {
                project: './tsconfig.json',
            },
        },
        plugins: {
            '@typescript-eslint': tseslint,
            react,
            'react-hooks': reactHooks,
            import: importPlugin,
            'unused-imports': unusedImports,
        },
        rules: {
            ...tseslint.configs['strict-type-checked'].rules,
            ...tseslint.configs['stylistic-type-checked'].rules,
            ...react.configs.recommended.rules,
            ...react.configs['jsx-runtime'].rules,
            ...reactHooks.configs.recommended.rules,
            ...importPlugin.configs.recommended.rules,
            ...importPlugin.configs.typescript.rules,
            'unused-imports/no-unused-imports': 'error',
            'import/order': [
                'error',
                {
                    groups: ['builtin', 'external', 'internal', 'parent', 'sibling', 'index'],
                    alphabetize: {
                        order: 'asc',
                        caseInsensitive: true,
                    },
                },
            ],
            'react/no-unknown-property': ['error', { ignore: ['css'] }],
            '@typescript-eslint/array-type': [
                'error',
                {
                    default: 'generic',
                },
            ],
        },
        settings: {
            'import/parsers': {
                '@typescript-eslint/parser': ['.ts', '.tsx'],
            },
            'import/resolver': {
                typescript: {
                    alwaysTryTypes: true,
                    project: './tsconfig.json',
                },
            },
            react: {
                version: 'detect',
            },
        },
    },

    {
        files: ['frontend/jest.setup.js'],
        languageOptions: {
            sourceType: 'commonjs',
        },
    },
];
