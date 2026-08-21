function main() {
    return {
        description: 'Single JavaScript rule',
        description_notes: ['example 1', 'example 2'],
        maintainers: ['tekezo'],
        manipulators: [
            {
                from: {
                    key_code: 'spacebar',
                },
                to: [
                    {
                        key_code: 'tab',
                    },
                ],
                type: 'basic',
            },
        ],
    }
}

main()
