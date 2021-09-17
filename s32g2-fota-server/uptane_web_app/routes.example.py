# -*- coding: utf-8 -*-

from fileutils import abspath
from languages import read_possible_languages

possible_languages = read_possible_languages(abspath('applications', app))

routers = {
    app: dict(
        default_language=possible_languages['default'][0],
        languages=[lang for lang in possible_languages if lang != 'default']
    )
}

