/*
 * Copyright 2021 Scott Bender <scott@scottbender.net>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

export default function (app: any) {
    const error = app.error
    const debug = app.debug
    let onStop: any = []
    let paths: any

    function filterPath (sourecPath: any, ps: any) {
        if (ps.wildcard) {
            return sourecPath.startsWith(ps.path)
        } else {
            return sourecPath == ps.path
        }
    }

    function deltaInputHandler (delta: any, next: any) {
        if (delta.updates && delta.context == app.selfContext) {
            delta.updates.forEach((update: any) => {
                if (update.values) {
                    let newValues: any = []
                    update.values.forEach((pathValue: any) => {
                        let matched = false
                        paths.forEach((ps: any) => {
                            if (
                                filterPath(pathValue.path, ps) &&
                                (!ps.source || (update.$source && ps.source == update.$source))
                            ) {
                                matched = true
                            }
                        })
                        if (!matched) {
                            newValues.push(pathValue)
                        }
                    })
                    update.values = newValues
                }
            })
        }
        next(delta)
    }

    const plugin: Plugin = {
        start: function (props: any) {
            if (props.paths && props.paths.length > 0) {
                paths = props.paths
                app.registerDeltaInputHandler(deltaInputHandler)
            }
        },

        stop: function () {
            onStop.forEach((f: any) => f())
            onStop = []
        },

        id: 'signalk-path-filter',
        name: 'Path Filter',
        description: 'Signal K Plugin which filters out paths',
        schema: {
            type: 'object',
            properties: {
                paths: {
                    title: 'Paths to filter out',
                    type: 'array',
                    items: {
                        type: 'object',
                        properties: {
                            path: {
                                type: 'string',
                                title: 'Path'
                            },
                            wildcard: {
                                type: 'boolean',
                                title: 'Wildcard',
                                description:
                                    'Filter all paths that start with the entered path',
                                default: false
                            },
                            source: {
                                type: 'string',
                                title: '$Source'
                            }
                        }
                    }
                }
            }
        }
    }

    return plugin
}

interface Plugin {
    start: (app: any) => void
    stop: () => void
    id: string
    name: string
    description: string
    schema: any
}