function frontendInit() {
    console.log('init');
    setTimeout(loadXMLDoc, 1000);
}

function loadXMLDoc() {
    console.log('loadXMLDoc');
    var xmlhttp = new XMLHttpRequest();

    xmlhttp.onreadystatechange = function () {
        console.log('onreadystatechange');
        if (xmlhttp.readyState === XMLHttpRequest.DONE) {   // XMLHttpRequest.DONE == 4
            if (xmlhttp.status === 200) {
                //document.getElementById("myDiv").innerHTML = xmlhttp.responseText;
                console.log(xmlhttp);
            } else {
                console.error(xmlhttp);
                alert('something else other than 200 was returned');
            }
        }
    };

    xmlhttp.open("GET", "http://192.168.1.82/state", true);
    xmlhttp.send();
}