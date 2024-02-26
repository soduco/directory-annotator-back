Table of contents

[[_TOC_]]

# Soduco API

```
curl -L -X GET http://<HOST>:<PORT>/directories/ -H "Authorization:<AUTH_TOKEN>" 
```

Pendant le développement, utiliser:

* HOST = `localhost`
* PORT = `8000`
* AUTH_TOKEN = `12345678`


## Exemple de route ##

>**Route**: /.../...

>**Méthode**: GET, POST

>**Description**: Description de l'action associée à la route.

**Réponse (JSON)**:
```
{
    content: {
        key: data
    }
}
```

| Champ | Description |
|:-----:|:-----------:|
|  key  |    data     |


# COMPUTE API #


## Calcul de l'image deskew ##

>**Route**: /directories/\<directory>/\<view>/image_deskew

>**Méthode**: GET

>**Description**: Récupère l'image deskewed sous format JPEG de la page `view` du pdf `directory`.


**Réponse **:
```
Renvoie l'image sous la forme d'un jpeg.
```
Exemple commande CURL:

`
curl -X GET http://localhost:5000/directories/Didot_1843a.pdf/700/image_deskew -H 'Authorization:12345678'
`
Attention: la valeur après les deux points dans Authorization est la valeur du Token. la valeur 12345678 n'est accepté qu'en mode débug.

Réponse CURL:
Renvoie l'image sous format binaire. Il est possible de la lire avec un logiciel (feh, gimp).

## Calcul automatique de l'annotation ##

>**Route**: /directories/\<directory>/\<view>/annotation?font-size=<int>&ocr-engine=<pero|tesseract> 

>**Méthode**: GET

>**Description**: Lance le calcul JSON de la page `view` du pdf `directory`.


**Réponse (JSON)**:
```
{
    "content": {}, 
    "mode": "computed"
}
```

| Champ     |                        Description                         |
|:-----:    |:----------------------------------------------------------:|
| content   | Dictionnaire qui stocke les informations du texte extrait. |
| mode      | Mode dans lequel la page a été chargée.                    |

Exemple commande CURL:


* `curl -X GET http://localhost:5000/directories/Didot_1843a.pdf/700/annotation -H 'Authorization:12345678'`
* `curl -X GET http://localhost:5000/directories/Didot_1843a.pdf/700/annotation?font-size=20 -H 'Authorization:12345678'`
* `curl -X GET http://localhost:5000/directories/Didot_1843a.pdf/700/annotation?ocr-engine=tesseract -H 'Authorization:12345678'`

Attention: la valeur après les deux points dans Authorization est la valeur du Token. la valeur 12345678 n'est accepté qu'en mode débug.

Réponse CURL:
```
{
    "content": [
        ...
    ]
    "mode": "computed"
}
```




# Pas implémenté

## Liste des vues modifiées ##
>Non prioritaire

>**Route**: /directories/\<directory>/views/\<view>

>**Méthode**: GET

>**Description**: Affiche la liste des views qui ont été modifiées dans le pdf `directory`.

**Réponse (JSON)**:
```
{
    "list" : {}
    "filename": directory
}
```

|  Champ   |  Description   |
|:--------:|:--------------:|
|   list   | Liste de views |
| filename |   directory    |

## Suggestions ##

* Système d'authentification ;
* Vérifier si le fichier a été modifié entre le moment où on l'a chargé et le moment où on le sauvegarde ;
* Savoir qui a modifié quelle page et à quel moment ;
* Permettre de savoir qui est en ce moment en ligne et sur quel fichier il travaille.
