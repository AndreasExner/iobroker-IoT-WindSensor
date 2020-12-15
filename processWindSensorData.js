
// process wind speed / guest

on({id: "0_userdata.0.IoT.WindSensor.WindSpeedArray", change: "any"}, function (obj) {
    var value = obj.state.val;
    var oldValue = obj.oldState.val;

    setState('0_userdata.0.IoT.Weather.WindSpeed', calc_PDF(value));
    setState('0_userdata.0.IoT.Weather.WindGuest', calc_MAX(value));

});

// process wind direction

on({id: "0_userdata.0.IoT.WindSensor.WindDirectionArray", change: "any"}, function (obj) {
    var value = obj.state.val;
    var oldValue = obj.oldState.val;

    var WindDirectionNumeric = calc_PDF(value);

    setState('0_userdata.0.IoT.Weather.WindDirectionNumeric', WindDirectionNumeric);
    setState('0_userdata.0.IoT.Weather.WindDirection', get_WindDirectionName(WindDirectionNumeric));
    
});


// ################################### get wind direction name

function get_WindDirectionName(WindDirectionNumeric) {

    switch (WindDirectionNumeric) {
        case 22: return "Nord-Nordost";
        case 45: return "Nordost"; 
        case 67: return "Ost-Nordost";
        case 90: return "Ost";
        case 112: return "Ost-Südost";
        case 135: return "Südost";
        case 157: return "Süd-Südost";
        case 180: return "Süd";
        case 202: return "Süd-Südwest";
        case 225: return "Südwest";
        case 247: return "West-Südwest";
        case 270: return "West";
        case 292: return "West-Nordwest";
        case 315: return "Nordwest";
        case 337: return "Nord-Nordwest";
        case 360: return "Nord";
    }
    return "n/a";
}


// ################################### get maximum

function calc_MAX(inputString) {

    var i = 0;

    // split string into an array and parse into an numeric array

    var StrArray = inputString.split(",");
    let NumArray = new Array(); 
    var length = StrArray.length;

    for (i = 0; i < length; i++ ) {
        NumArray[i] = parseFloat(StrArray[i]);
    };

    return (Math.max.apply(Math, NumArray));
}

// ################################### calc mean value (normal distribution)

function calc_PDF(inputString) {

    var i = 0;

    // split string into an array and parse into an numeric array

    var StrArray = inputString.split(",");
    let NumArray = new Array(); 
    var length = StrArray.length;

    for (i = 0; i < length; i++ ) {
        NumArray[i] = parseFloat(StrArray[i]);
    };

    // calculate the arithmetic mean
    
    var sum = 0;

    for (i = 0; i < length; i++ ) {
        sum += NumArray[i];
    };
    
    var arMean = sum / length;

    // calculate the variant and standard deviation

    var x = 0;
    
    for (i = 0; i < length; i++ ) {
        x += Math.pow(NumArray[i] - arMean, 2);
    };

    var varinat = 1 / (length - 1) * x;
    var stdDev = Math.sqrt(varinat);
    
    // calculate the normal distribution (Probability Density Function)

    let PDFArray = new Array();
    let PDFparam = new Array();
        
    PDFparam[0] = arMean;
    PDFparam[1] = stdDev;
    PDFArray = pdf("norm" ,NumArray, PDFparam); 

    // get the median and the predicted value

    var median = Math.max.apply(Math, PDFArray);
    const medianNumber = (element) => element == median;

    var index = PDFArray.findIndex(medianNumber);

    //for debugging
 /*
    var max = Math.max.apply(Math, NumArray);
    var min = Math.min.apply(Math, NumArray);

    console.log("sum = " + sum + ", arMean = " + arMean + ", varinat = " + varinat + ", stdDev = " + stdDev + ", median = " + median + ", index = " + index);
*/
    return(NumArray[index]);
}


// ################################### PDF algorithem (University of Utah)

function pdf(type,xpdft,paramt) {

    var ypdf = new Array;

    if (xpdft.constructor != Array) { 

        var xpdf = new Array;
        xpdf[0] = xpdft; 
    }
    else {xpdf = xpdft;}
    
    if (paramt.constructor != Array) {
        
        var param = new Array; 
        param[0] = paramt; 
    }
    else {param = paramt}


    if (type=='norm') {    //normal, gaussian distribution
        
        if (param == null) {param = new Array(0,1);}
        
        var c1 = Math.sqrt(1 / (2 * Math.PI)) / param[1];
        
        var c2 = 1 / (2 * param[1] * param[1]);
        
        for ( var ip = 0; ip < xpdf.length; ip++ ){

            ypdf[ip] = c1 * Math.exp(-(xpdf[ip] - param[0]) * (xpdf[ip] - param[0]) * c2);
        }
    }

    return ypdf;
}
